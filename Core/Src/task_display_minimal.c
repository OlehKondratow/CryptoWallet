/**
  ******************************************************************************
  * @file    task_display_minimal.c
  * @brief   Reduced display task for `minimal-lwip` — faster Ethernet-first bring-up.
  ******************************************************************************
  * @details
  *          **Purpose:** Same logging hook as full UI but minimal OLED/state machine load
  *          while validating LwIP. Swapped in by Makefile instead of @c task_display.c .
  *          @c SKIP_OLED=1 skips I2C if bus hangs. **Production UI:** @c task_display.c .
  ******************************************************************************
  */

#include "main.h"
#include "hw_init.h"
#include "task_display.h"
#include "wallet_shared.h"
#include "ssd1306.h"
#include "ssd1306_fonts.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <string.h>

#define DISP_STACK  512U
#define DISP_PRIO   (tskIDLE_PRIORITY + 2)  /**< Lower than Net — network init first (like lwip-uaid OledThread) */

/** Set in Makefile: make minimal-lwip SKIP_OLED=1 — skip OLED if I2C hangs */
#ifndef SKIP_OLED
#define SKIP_OLED 0
#endif

static void display_task(void *pvParameters);

#define LOG_LINE_CHARS  21U  /**< SSD1306 128px / 6px per char */

/**
 * @brief   Mirror log to UART (CRLF) and copy printable tail into @c g_display_ctx.log_line .
 * @details Takes @c g_display_ctx_mutex when available. Safe to call from multiple tasks; short timeout.
 * @param   msg  Null-terminated ASCII message.
 */
void Task_Display_Log(const char *msg)
{
    if (msg != NULL) {
        UART_Log(msg);
        UART_Log("\r\n");
        if (g_display_ctx_mutex != NULL &&
            xSemaphoreTake(g_display_ctx_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            size_t dst = 0;
            for (size_t src = 0; msg[src] != '\0' && dst < sizeof(g_display_ctx.log_line) - 1U; src++) {
                char c = msg[src];
                if (c == '\r' || c == '\n') c = ' ';
                if ((unsigned char)c >= 0x20U && (unsigned char)c <= 0x7EU) {
                    g_display_ctx.log_line[dst++] = c;
                }
            }
            g_display_ctx.log_line[dst] = '\0';
            xSemaphoreGive(g_display_ctx_mutex);
        }
    }
}

/**
 * @brief   Spawn @c display_task (stack @c DISP_STACK , priority @c DISP_PRIO ).
 * @details Used in @c minimal-lwip build instead of full @c task_display.c implementation.
 */
void Task_Display_Create(void)
{
    xTaskCreate(display_task, "Disp", DISP_STACK, NULL, DISP_PRIO, NULL);
}

/**
 * @brief   No-op in minimal build (queue UI path unused).
 * @return  Always @c pdTRUE .
 */
BaseType_t UI_UpdateData(const UI_Display_Data_t *new_data)
{
    (void)new_data;
    return pdTRUE;
}

/**
 * @brief Stub - no-op for minimal build.
 */
void UI_ClearPending(void)
{
}

/**
 * @brief   Minimal OLED refresh loop: banner, optional IP string, log line.
 * @details When @c SKIP_OLED , skips I2C traffic. Lower priority than net task so Ethernet comes up first.
 * @param   pvParameters  Unused.
 */
static void display_task(void *pvParameters)
{
    (void)pvParameters;
    Task_Display_Log("Disp start");
    vTaskDelay(pdMS_TO_TICKS(100));

#if !SKIP_OLED
    /* OLED already inited in main (before scheduler) — same as lwip-uaid-SSD1306 */
    UART_Log("Disp: OLED OK\r\n");  /* Direct UART for diagnostics */
    Task_Display_Log("Disp: OLED OK");
    if (g_i2c_mutex != NULL && xSemaphoreTake(g_i2c_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        ssd1306_SetCursor(0, 8);
        ssd1306_WriteString("+ LwIP", Font_6x8, White);
        ssd1306_SetCursor(0, 16);
        ssd1306_WriteString("DHCP...", Font_6x8, White);
        ssd1306_UpdateScreen();
        xSemaphoreGive(g_i2c_mutex);
    }
#else
    Task_Display_Log("Disp: OLED skipped (SKIP_OLED=1)");
#endif

    TickType_t last_log = xTaskGetTickCount();
    for (;;) {
        HAL_GPIO_TogglePin(LED1_GPIO_PORT, LED1_PIN);
#if LWIP_ALIVE_LOG
        if ((xTaskGetTickCount() - last_log) >= pdMS_TO_TICKS(5000)) {
            Task_Display_Log("Disp: alive");
            last_log = xTaskGetTickCount();
        }
#endif
#if !SKIP_OLED
        if (g_i2c_mutex != NULL && xSemaphoreTake(g_i2c_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            if (g_display_ctx_mutex != NULL && xSemaphoreTake(g_display_ctx_mutex, pdMS_TO_TICKS(20)) == pdTRUE) {
                ssd1306_SetCursor(0, 16);
                ssd1306_WriteString(g_display_ctx.ip_addr[0] ? g_display_ctx.ip_addr : "DHCP...", Font_6x8, White);
                ssd1306_SetCursor(0, 24);
                {
                    char disp[LOG_LINE_CHARS + 1];
                    size_t len = 0;
                    while (g_display_ctx.log_line[len] != '\0' && len < LOG_LINE_CHARS) len++;
                    (void)memcpy(disp, g_display_ctx.log_line, len);
                    for (; len < LOG_LINE_CHARS; len++) disp[len] = ' ';
                    disp[LOG_LINE_CHARS] = '\0';
                    ssd1306_WriteString(disp, Font_6x8, White);
                }
                xSemaphoreGive(g_display_ctx_mutex);
            }
            ssd1306_UpdateScreen();
            xSemaphoreGive(g_i2c_mutex);
        }
#endif
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
