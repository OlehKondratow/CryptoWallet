/**
 * @file cwup_uart.c
 * @brief CWUP-0.1 minimal command set on USART3 (RX IT + line queue).
 * @note Not built when USE_RNG_DUMP=1 (binary TRNG stream on same UART).
 *
 * Phases (do not mix with @c USE_RNG_DUMP raw binary build):
 * - **A:** Until the first @c CW+READY line — no §7 TRNG frames; optional TEXT from other tasks (e.g. display log) is allowed.
 * - **B:** The single @c CW+READY,...\\r\\n line (emitted once at CWUP task start).
 * - **C:** TEXT command mode (@c AT+ / @c CW+).
 * - **D:** Framed TRNG (§7) after @c AT+RNG=START — not implemented here; reserved.
 */

#include "main.h"
#include "hw_init.h"
#include "fw_integrity.h"
#include "cwup_uart.h"
#include "cwup_wallet_probe.h"
#include "stm32h7xx_hal.h"
#include "stm32h7xx_hal_uart.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include <string.h>
#include <stdio.h>

#ifndef USE_RNG_DUMP

#ifndef CW_GIT_VERSION
#define CW_GIT_VERSION "unknown"
#endif

#ifndef CW_VERIFIED_BOOT
#define CW_VERIFIED_BOOT 0
#endif

/**
 * @brief 1 when framed TRNG via @c AT+RNG=START (§7) is implemented and available in this build.
 * @note Not the same as @c USE_RNG_DUMP (raw stream, CWUP disabled). Not tied to @c USE_CRYPTO_SIGN.
 */
#ifndef CWUP_RNG_FRAMED_AVAILABLE
#define CWUP_RNG_FRAMED_AVAILABLE 0
#endif

#define CWUP_LINE_MAX        256U
#define CWUP_QUEUE_DEPTH     4U
#define CWUP_TASK_STACK      768U
#define CWUP_PROTO           "CWUP/0.1"

extern UART_HandleTypeDef huart3;

static QueueHandle_t s_line_q;
static TaskHandle_t s_cwup_task;
static uint8_t s_rx_byte;
static char s_asm[CWUP_LINE_MAX];
static size_t s_asm_len;
static volatile cwup_line_phase_t s_line_phase = CWUP_PHASE_A_PRE_READY;

static int cwup_rng_proto_advertise(void)
{
#if CWUP_RNG_FRAMED_AVAILABLE
    return 1;
#else
    return 0;
#endif
}

static void cwup_send_crlf(const char *line)
{
    if (line == NULL) {
        return;
    }
    UART_Tx_Lock();
    (void)HAL_UART_Transmit(&huart3, (const uint8_t *)line, (uint16_t)strlen(line), 500);
    (void)HAL_UART_Transmit(&huart3, (const uint8_t *)"\r\n", 2U, 50);
    UART_Tx_Unlock();
}

#define CWUP_ECHO_PREFIX_LEN 8U /* "at+echo=" */

static int cwup_prefix_ci(const char *s, const char *low, size_t n)
{
    size_t i;

    for (i = 0U; i < n; i++) {
        char c = s[i];
        if (c >= 'A' && c <= 'Z') {
            c = (char)(c - 'A' + 'a');
        }
        if (c != low[i]) {
            return 0;
        }
    }
    return 1;
}

static void cwup_handle_echo_payload(const char *payload)
{
    char out[CWUP_LINE_MAX];
    char line[CWUP_LINE_MAX];
    size_t i;
    size_t o = 0U;

    if (payload == NULL) {
        payload = "";
    }
    for (i = 0U; payload[i] != '\0' && o < 120U; i++) {
        unsigned char c = (unsigned char)payload[i];
        if (c >= 0x20U && c <= 0x7EU) {
            out[o++] = (char)c;
        }
    }
    out[o] = '\0';
    (void)snprintf(line, sizeof(line), "CW+ECHO,%.120s", out);
    cwup_send_crlf(line);
}

static void cwup_send_ready_banner(void)
{
    char buf[160];
    const int rng_flag = cwup_rng_proto_advertise();

    (void)snprintf(buf, sizeof(buf),
                   "CW+READY,proto=%s,build=%s,rng=%d",
                   CWUP_PROTO, CW_GIT_VERSION, rng_flag);
    cwup_send_crlf(buf);
}

static void cwup_handle_line(const char *line)
{
    char work[CWUP_LINE_MAX];
    size_t i;
    size_t len;

    if (line == NULL) {
        return;
    }
    (void)strncpy(work, line, sizeof(work) - 1U);
    work[sizeof(work) - 1U] = '\0';
    for (i = 0; work[i] == ' ' || work[i] == '\t'; i++) {}
    if (i > 0U && i < sizeof(work)) {
        memmove(work, work + i, strlen(work + i) + 1U);
    }
    len = strlen(work);
    if (len >= CWUP_ECHO_PREFIX_LEN &&
        cwup_prefix_ci(work, "at+echo=", CWUP_ECHO_PREFIX_LEN) != 0) {
        cwup_handle_echo_payload(work + CWUP_ECHO_PREFIX_LEN);
        return;
    }

    if (len < 4U) {
        return;
    }
    for (i = 0; i < len && i < CWUP_LINE_MAX; i++) {
        char c = work[i];
        if (c >= 'A' && c <= 'Z') {
            c = (char)(c - 'A' + 'a');
        }
        work[i] = c;
    }

    if (strcmp(work, "at+ping") == 0) {
        cwup_send_crlf("CW+PONG");
        return;
    }
    if (strcmp(work, "at+cwinfo?") == 0) {
        char r[192];
        (void)snprintf(r, sizeof(r),
                       "CW+CWINFO,proto=%s,build=%s",
                       CWUP_PROTO, CW_GIT_VERSION);
        cwup_send_crlf(r);
        return;
    }
    if (strcmp(work, "at+ready?") == 0) {
        char r[192];
        const int rng_flag = cwup_rng_proto_advertise();

        (void)snprintf(r, sizeof(r),
                       "CW+READY,proto=%s,build=%s,rng=%d",
                       CWUP_PROTO, CW_GIT_VERSION, rng_flag);
        cwup_send_crlf(r);
        return;
    }
    if (strcmp(work, "at+fwinfo?") == 0) {
        char info[120];
        char r[140];
        if (fw_integrity_snprint(info, sizeof(info)) > 0) {
            (void)snprintf(r, sizeof(r), "CW+FWINFO,%s", info);
            cwup_send_crlf(r);
        } else {
            cwup_send_crlf("CW+ERR=20,fwinfo");
        }
        return;
    }
    if (strcmp(work, "at+bootchain?") == 0) {
        char r[200];
        uint32_t vtor = SCB->VTOR;
        uint32_t entry = 0U;
        if (vtor != 0U) {
            entry = *(const volatile uint32_t *)(vtor + 4U);
        }
        (void)snprintf(r, sizeof(r),
                       "CW+BOOTCHAIN,rom=STM32,verified_boot=%d,app_entry=0x%08lX,note=app+stm32_secure_boot",
                       (int)CW_VERIFIED_BOOT, (unsigned long)entry);
        cwup_send_crlf(r);
        return;
    }
    if (strcmp(work, "at+wallet?") == 0) {
        char r[160];
        int seed_on = cwup_wallet_probe_seed_available();
#ifdef USE_CRYPTO_SIGN
        const int cs = 1;
#else
        const int cs = 0;
#endif
        (void)snprintf(r, sizeof(r), "CW+WALLET,seed=%d,crypto_sign=%d", seed_on, cs);
        cwup_send_crlf(r);
        return;
    }
    if (strcmp(work, "at+selftest?") == 0) {
        char r[120];
        (void)snprintf(r, sizeof(r), "CW+SELFTEST,ok=1,tick=%lu",
                       (unsigned long)xTaskGetTickCount());
        cwup_send_crlf(r);
        return;
    }

    cwup_send_crlf("CW+ERR=1,unknown command");
}

static void cwup_start_rx(void)
{
    HAL_NVIC_SetPriority(USART3_IRQn, 6, 0U);
    HAL_NVIC_EnableIRQ(USART3_IRQn);
    if (HAL_UART_Receive_IT(&huart3, &s_rx_byte, 1U) != HAL_OK) {
        /* RX not started — CWUP commands unavailable */
    }
}

static void cwup_task(void *arg)
{
    char line[CWUP_LINE_MAX];

    (void)arg;
    s_line_phase = CWUP_PHASE_A_PRE_READY;
    cwup_start_rx();
    cwup_send_ready_banner();
    s_line_phase = CWUP_PHASE_C_TEXT_CMD;

    for (;;) {
        if (xQueueReceive(s_line_q, line, portMAX_DELAY) == pdTRUE) {
            cwup_handle_line(line);
        }
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    BaseType_t hpw = pdFALSE;
    uint8_t b;

    if (huart != &huart3) {
        return;
    }

    b = s_rx_byte;
    (void)HAL_UART_Receive_IT(&huart3, &s_rx_byte, 1U);

    if (b == (uint8_t)'\r' || b == (uint8_t)'\n') {
        if (s_asm_len > 0U) {
            s_asm[s_asm_len] = '\0';
            if (s_line_q != NULL) {
                (void)xQueueSendFromISR(s_line_q, s_asm, &hpw);
            }
            s_asm_len = 0U;
        }
    } else if (s_asm_len < CWUP_LINE_MAX - 1U) {
        s_asm[s_asm_len++] = (char)b;
    } else {
        s_asm_len = 0U;
    }

    portYIELD_FROM_ISR(hpw);
}

cwup_line_phase_t Cwup_GetLinePhase(void)
{
    return s_line_phase;
}

void Cwup_Init(void)
{
    s_line_q = xQueueCreate(CWUP_QUEUE_DEPTH, CWUP_LINE_MAX);
    if (s_line_q == NULL) {
        return;
    }
    if (xTaskCreate(cwup_task, "CWUP", CWUP_TASK_STACK, NULL,
                    tskIDLE_PRIORITY + 3U, &s_cwup_task) != pdPASS) {
        return;
    }
    (void)s_cwup_task;
}

#else /* USE_RNG_DUMP */

void Cwup_Init(void) {}

#endif /* USE_RNG_DUMP */
