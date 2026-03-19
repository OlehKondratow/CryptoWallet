/**
  ******************************************************************************
  * @file    task_display.c
  * @brief   SSD1306 128×32 — four scroll lines, state machine, queue-driven UI.
  ******************************************************************************
  * @details
  *          **Inputs:** @c g_display_queue (@c Transaction_Data_t from net),
  *          @c g_display_ctx + @c g_display_ctx_mutex (signing/net status),
  *          @c Task_Display_Log() — **UART + on-screen log** (architecture: logging hub).
  *
  *          **UI states:** @c UI_State_t / @c display_state_t — WALLET, SECURITY,
  *          NETWORK, LOG (amount, lock/sig, IP/MAC, scroll buffer).
  *
  *          **Concurrency:** @c g_i2c_mutex wraps I2C; @c g_ui_mutex for merged UI data.
  *          **Clean-code note:** @c display_task / @c render_four_scroll_lines are long;
  *          acceptable for embedded; could be split further.
  *
  *          **Docs:** @c docs_src/architecture.md , @c README.md .
  ******************************************************************************
  */

#include "task_display.h"
#include "main.h"
#include "hw_init.h"
#include "wallet_shared.h"
#include "ssd1306.h"
#include "ssd1306_fonts.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include <string.h>
#include <stdio.h>

#define DISPLAY_STACK_SIZE    512U
#define DISPLAY_PRIORITY      (tskIDLE_PRIORITY + 3)  /* Higher than Net so display inits first */
#define LOG_SCROLL_MS        120U
#define LOG_BUF_SIZE         384U
#define CHARS_PER_LINE       21U
#define MAX_LINE_LEN         80U
#define NUM_LINES            4U
#define QUEUE_WAIT_MS        100U

static void display_task(void *pvParameters);
static void render_four_scroll_lines(void);
static void build_line_buffers(void);
static void display_lock(void);
static void display_unlock(void);

/** @brief Global UI state (merged from queue + UI_UpdateData) */
static UI_Display_Data_t s_ui_data;
static char s_log_buf[LOG_BUF_SIZE];
static uint16_t s_log_len;

/** @brief 4 scrolling lines: buf, len, scroll_offset */
static char s_line_buf[NUM_LINES][MAX_LINE_LEN];
static uint16_t s_line_len[NUM_LINES];
static uint16_t s_line_offset[NUM_LINES];

/**
 * @brief Create and start the display task (wrapper).
 * @return None.
 */
void Task_Display_Create(void)
{
    xTaskCreate(display_task, "Display", DISPLAY_STACK_SIZE, NULL,
                DISPLAY_PRIORITY, NULL);
}

/**
 * @brief Entry point for the Display Task.
 * @param argument FreeRTOS task argument (unused).
 * @return None.
 */
void StartDisplayTask(void *argument)
{
    display_task(argument);
}

/**
 * @brief Updates the display state via merge into s_ui_data.
 * @param new_data Pointer to the new UI data structure.
 * @return pdTRUE if updated, pdFALSE on mutex timeout.
 */
BaseType_t UI_UpdateData(const UI_Display_Data_t *new_data)
{
    if (new_data == NULL || g_ui_mutex == NULL) return pdFALSE;
    if (xSemaphoreTake(g_ui_mutex, pdMS_TO_TICKS(50)) != pdTRUE) return pdFALSE;

    if (new_data->ip_addr[0] != '\0') {
        (void)strncpy(s_ui_data.ip_addr, new_data->ip_addr, sizeof(s_ui_data.ip_addr) - 1);
        s_ui_data.ip_addr[sizeof(s_ui_data.ip_addr) - 1] = '\0';
    }
    if (new_data->mac_addr[0] != '\0') {
        (void)strncpy(s_ui_data.mac_addr, new_data->mac_addr, sizeof(s_ui_data.mac_addr) - 1);
        s_ui_data.mac_addr[sizeof(s_ui_data.mac_addr) - 1] = '\0';
    }
    if (new_data->last_log[0] != '\0') {
        (void)strncpy(s_ui_data.last_log, new_data->last_log, sizeof(s_ui_data.last_log) - 1);
        s_ui_data.last_log[sizeof(s_ui_data.last_log) - 1] = '\0';
    }
    s_ui_data.is_safe_locked = new_data->is_safe_locked;
    s_ui_data.current_state = new_data->current_state;
    s_ui_data.pending_tx = new_data->pending_tx;

    xSemaphoreGive(g_ui_mutex);
    return pdTRUE;
}

/**
 * @brief Append a message to the scrollable system log.
 * @param msg  Null-terminated string (truncated if exceeds buffer).
 * @return None.
 */
void Task_Display_Log(const char *msg)
{
    if (msg == NULL) return;
    UART_Log(msg);
    UART_Log("\r\n");
    if (xSemaphoreTake(g_display_ctx_mutex, pdMS_TO_TICKS(50)) != pdTRUE) return;

    size_t n = strlen(msg);
    if (n > (LOG_BUF_SIZE - s_log_len - 4)) {
        n = (LOG_BUF_SIZE - s_log_len - 4);
    }
    if (n > 0) {
        if (s_log_len > 0) {
            s_log_buf[s_log_len++] = ' ';
            s_log_buf[s_log_len++] = '|';
            s_log_buf[s_log_len++] = ' ';
        }
        for (size_t i = 0; i < n && s_log_len < LOG_BUF_SIZE - 1; i++) {
            char c = msg[i];
            if (c == '\r' || c == '\n') c = ' ';
            if ((unsigned char)c >= 0x20 && (unsigned char)c <= 0x7E) {
                s_log_buf[s_log_len++] = c;
            }
        }
        s_log_buf[s_log_len] = '\0';
    }

    if (g_ui_mutex != NULL && xSemaphoreTake(g_ui_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        (void)strncpy(s_ui_data.last_log, msg, sizeof(s_ui_data.last_log) - 1);
        s_ui_data.last_log[sizeof(s_ui_data.last_log) - 1] = '\0';
        xSemaphoreGive(g_ui_mutex);
    }
    xSemaphoreGive(g_display_ctx_mutex);
}

/**
 * @brief Clear the pending transaction flag.
 * @return None.
 */
void UI_ClearPending(void)
{
    if (g_ui_mutex == NULL) return;
    if (xSemaphoreTake(g_ui_mutex, pdMS_TO_TICKS(50)) != pdTRUE) return;
    s_ui_data.pending_tx.is_pending = 0;
    xSemaphoreGive(g_ui_mutex);
}

/**
 * @brief Acquire I2C mutex for display access.
 * @return None.
 */
static void display_lock(void)
{
    xSemaphoreTake(g_i2c_mutex, portMAX_DELAY);
}

/**
 * @brief Release I2C mutex.
 * @return None.
 */
static void display_unlock(void)
{
    xSemaphoreGive(g_i2c_mutex);
}

/**
 * @brief Build content for 4 scrolling lines from s_ui_data and s_log_buf.
 */
static void build_line_buffers(void)
{
    UI_Display_Data_t ui;
    if (g_ui_mutex != NULL && xSemaphoreTake(g_ui_mutex, pdMS_TO_TICKS(20)) == pdTRUE) {
        ui = s_ui_data;
        xSemaphoreGive(g_ui_mutex);
    } else {
        memset(&ui, 0, sizeof(ui));
        (void)strncpy(ui.pending_tx.coin_name, "BTC", 7);
        ui.is_safe_locked = 1;
    }

    /* Line 0: WALLET */
    if (ui.pending_tx.is_pending) {
        size_t rlen = strlen(ui.pending_tx.recipient);
        const char *r = ui.pending_tx.recipient;
        if (rlen > 12) {
            (void)snprintf(s_line_buf[0], MAX_LINE_LEN, "WALLET | %s: %.6g -> %.6s..%.6s | Confirm? ",
                           ui.pending_tx.coin_name[0] ? ui.pending_tx.coin_name : "BTC",
                           ui.pending_tx.amount, r, r + rlen - 6);
        } else {
            (void)snprintf(s_line_buf[0], MAX_LINE_LEN, "WALLET | %s: %.6g -> %s | Confirm? ",
                           ui.pending_tx.coin_name[0] ? ui.pending_tx.coin_name : "BTC",
                           ui.pending_tx.amount, rlen > 0 ? r : "addr");
        }
    } else {
        (void)snprintf(s_line_buf[0], MAX_LINE_LEN, "WALLET | %s: %.6g | Secure Wallet ",
                       ui.pending_tx.coin_name[0] ? ui.pending_tx.coin_name : "BTC",
                       ui.pending_tx.amount);
    }
    s_line_len[0] = (uint16_t)strlen(s_line_buf[0]);
    if (s_line_len[0] < CHARS_PER_LINE) {
        size_t pad = CHARS_PER_LINE - s_line_len[0];
        for (size_t i = 0; i < pad && s_line_len[0] < MAX_LINE_LEN - 1; i++) {
            s_line_buf[0][s_line_len[0]++] = ' ';
        }
        s_line_buf[0][s_line_len[0]] = '\0';
    }

    /* Line 1: SECURITY */
    (void)snprintf(s_line_buf[1], MAX_LINE_LEN, "SECURITY | %s | Sig: -- ",
                   ui.is_safe_locked ? "Safe: Locked" : "Safe: Unlocked");
    s_line_len[1] = (uint16_t)strlen(s_line_buf[1]);

    /* Line 2: NETWORK */
    (void)snprintf(s_line_buf[2], MAX_LINE_LEN, "NETWORK | IP: %s | MAC: %s ",
                   ui.ip_addr[0] ? ui.ip_addr : "--",
                   ui.mac_addr[0] ? ui.mac_addr : "--");
    s_line_len[2] = (uint16_t)strlen(s_line_buf[2]);

    /* Line 3: LOG (from s_log_buf) */
    if (xSemaphoreTake(g_display_ctx_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        if (s_log_len > 0) {
            size_t copy = (s_log_len < MAX_LINE_LEN - 2) ? s_log_len : MAX_LINE_LEN - 2;
            memcpy(s_line_buf[3], s_log_buf, copy);
            s_line_buf[3][copy] = ' ';
            s_line_buf[3][copy + 1] = '\0';
        } else {
            (void)strncpy(s_line_buf[3], "LOG | Ready ", MAX_LINE_LEN - 1);
            s_line_buf[3][MAX_LINE_LEN - 1] = '\0';
        }
        xSemaphoreGive(g_display_ctx_mutex);
    }
    s_line_len[3] = (uint16_t)strlen(s_line_buf[3]);
}

/**
 * @brief Render 4 scrolling lines (marquee/ticker).
 */
static void render_four_scroll_lines(void)
{
    build_line_buffers();

    display_lock();
    ssd1306_Fill(Black);

    for (uint8_t row = 0; row < NUM_LINES; row++) {
        char window[CHARS_PER_LINE + 1];
        uint16_t len = s_line_len[row];
        uint16_t off = s_line_offset[row];
        /* Lines 0 and 1 (WALLET, SECURITY): static, no scroll */
        if (row <= 1) {
            off = 0;
        }
        for (uint16_t i = 0; i < CHARS_PER_LINE; i++) {
            window[i] = (len > 0) ? s_line_buf[row][(off + i) % len] : ' ';
        }
        window[CHARS_PER_LINE] = '\0';
        /* Only lines 2 and 3 scroll */
        if (row >= 2 && len > 0) {
            s_line_offset[row] = (s_line_offset[row] + 1) % len;
        }

        ssd1306_SetCursor(0, row * 8);
        ssd1306_WriteString(window, Font_6x8, White);
    }
    ssd1306_UpdateScreen();
    display_unlock();
}

/**
 * @brief Display task: 4 scrolling lines, queue for new tx.
 */
static void display_task(void *pvParameters)
{
    (void)pvParameters;
    memset(&s_ui_data, 0, sizeof(s_ui_data));
    memset(s_line_buf, 0, sizeof(s_line_buf));
    memset(s_line_len, 0, sizeof(s_line_len));
    memset(s_line_offset, 0, sizeof(s_line_offset));
    (void)strncpy(s_ui_data.pending_tx.coin_name, "BTC", 7);
    s_ui_data.is_safe_locked = 1;

    vTaskDelay(pdMS_TO_TICKS(300));  /* Allow power-up; display init needs I2C ready */

    display_lock();
    ssd1306_Init();
    ssd1306_Fill(Black);
    ssd1306_UpdateScreen();
    display_unlock();

    Task_Display_Log("Display init");

    for (;;) {
        Transaction_Data_t tx;
        if (xQueueReceive(g_display_queue, &tx, pdMS_TO_TICKS(QUEUE_WAIT_MS)) == pdTRUE) {
            if (g_ui_mutex != NULL && xSemaphoreTake(g_ui_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
                s_ui_data.pending_tx = tx;
                s_ui_data.pending_tx.is_pending = 1;
                xSemaphoreGive(g_ui_mutex);
            }
            Task_Display_Log("TX recv");
        }

        render_four_scroll_lines();
        vTaskDelay(pdMS_TO_TICKS(LOG_SCROLL_MS));
    }
}
