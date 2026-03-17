/**
  ******************************************************************************
  * @file    task_display_minimal.c
  * @brief   Minimal display task for minimal+LwIP staged build.
  ******************************************************************************
  * @details Simple LED+OLED loop, Task_Display_Log stub. Replaces full
  *          task_display.c when building minimal-lwip.
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

/**
 * @brief Append log message to UART (stub for minimal build).
 * @param msg Null-terminated string.
 */
void Task_Display_Log(const char *msg)
{
    if (msg != NULL) {
        UART_Log(msg);
        UART_Log("\r\n");
    }
}

/**
 * @brief Create and start the minimal display task.
 */
void Task_Display_Create(void)
{
    xTaskCreate(display_task, "Disp", DISP_STACK, NULL, DISP_PRIO, NULL);
}

/**
 * @brief Stub - no-op for minimal build.
 * @param new_data Unused.
 * @return pdTRUE.
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
 * @brief Display task - LED blink, OLED init, IP display.
 * @param pvParameters Unused.
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
                xSemaphoreGive(g_display_ctx_mutex);
            }
            ssd1306_UpdateScreen();
            xSemaphoreGive(g_i2c_mutex);
        }
#endif
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
