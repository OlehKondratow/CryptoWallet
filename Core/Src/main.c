/**
  ******************************************************************************
  * @file    main.c
  * @brief   CryptoWallet — FreeRTOS + LwIP + display (STM32H743ZI2).
  ******************************************************************************
  * @details MPU + Cache before HAL_Init (lwip_zero order). Display + Net tasks.
  *          BOOT_TEST: diagnostic without FreeRTOS. SKIP_OLED: skip I2C/OLED.
  ******************************************************************************
  */

#include "main.h"
#include "hw_init.h"
#include "time_service.h"
#include "wallet_shared.h"
#include "task_display.h"
#include "task_net.h"
#ifndef SKIP_OLED
#define SKIP_OLED 0
#endif
#if !SKIP_OLED
#include "ssd1306.h"
#include "ssd1306_fonts.h"
#endif
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"
#include "semphr.h"
#include "cmsis_os2.h"

/*-----------------------------------------------------------------------------
 * Global handles (wallet_shared.h)
 *-----------------------------------------------------------------------------*/
QueueHandle_t      g_tx_queue = NULL;
QueueHandle_t      g_display_queue = NULL;
EventGroupHandle_t g_user_event_group = NULL;
SemaphoreHandle_t g_i2c_mutex = NULL;
SemaphoreHandle_t g_ui_mutex = NULL;
display_context_t g_display_ctx = {0};
SemaphoreHandle_t g_display_ctx_mutex = NULL;
volatile uint8_t  g_security_alert = 0;

/**
 * @brief Fatal error handler - log and infinite loop.
 * @return Never returns.
 */
void Error_Handler(void)
{
    UART_Log("[ERR]\r\n");
    for (;;) {}
}

/**
 * @brief FreeRTOS hook when pvPortMalloc fails.
 * @return Never returns.
 */
void vApplicationMallocFailedHook(void)
{
    UART_Log("[MALLOC FAIL]\r\n");
    for (;;) {}
}

/**
 * @brief FreeRTOS hook on task stack overflow.
 * @param xTask     Task handle (unused).
 * @param pcTaskName Task name for log.
 * @return Never returns.
 */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    UART_Log("[STACK OVF ");
    if (pcTaskName != NULL) UART_Log(pcTaskName);
    UART_Log("]\r\n");
    for (;;) {}
}

/**
 * @brief Application entry point.
 * @return Never returns.
 */
int main(void)
{
    /* LwIP may access unaligned protocol fields on Cortex-M7 */
    SCB->CCR &= ~SCB_CCR_UNALIGN_TRP_Msk;

    /* Same init order as lwip_zero: MPU + Cache BEFORE HAL_Init */
    HW_Init_Early_LwIP();
    HAL_Init();
    HW_Init();

    time_service_init();
    Task_Display_Log("CryptoWallet + LwIP");
    HAL_GPIO_WritePin(LED1_GPIO_PORT, LED1_PIN, LED1_ON_LEVEL);
    HAL_GPIO_WritePin(LED2_GPIO_PORT, LED2_PIN, LED2_OFF_LEVEL);
    HAL_GPIO_WritePin(LED3_GPIO_PORT, LED3_PIN, LED3_OFF_LEVEL);

#ifdef BOOT_TEST
    /* Diagnostic: no FreeRTOS, just blink LED. Use: make boottest */
    for (;;) {
        HAL_GPIO_TogglePin(LED1_GPIO_PORT, LED1_PIN);
        Task_Display_Log("boot");
        HAL_Delay(500);
    }
#endif

    g_tx_queue = xQueueCreate(4U, sizeof(wallet_tx_t));
    g_display_queue = xQueueCreate(4U, sizeof(Transaction_Data_t));
    g_user_event_group = xEventGroupCreate();
    g_i2c_mutex = xSemaphoreCreateMutex();
    g_ui_mutex = xSemaphoreCreateMutex();
    g_display_ctx_mutex = xSemaphoreCreateMutex();

    if (g_tx_queue == NULL || g_display_queue == NULL || g_user_event_group == NULL ||
        g_i2c_mutex == NULL || g_ui_mutex == NULL || g_display_ctx_mutex == NULL) {
        Error_Handler();
    }
    Task_Display_Log("main: queues OK");

    /* Same as lwip-uaid-SSD1306: osKernelInitialize before creating threads */
    if (osKernelInitialize() != osOK) {
        Task_Display_Log("[ERR] osKernelInit");
        Error_Handler();
    }
    Task_Display_Create();
    Task_Net_Create();
    Task_Display_Log("main: tasks created, starting scheduler");

#if !SKIP_OLED
    /* OLED already inited in HW_Init (right after I2C). Show startup banner. */
    ssd1306_Fill(Black);
    ssd1306_SetCursor(0, 0);
    ssd1306_WriteString("CryptoWallet", Font_6x8, White);
    ssd1306_SetCursor(0, 8);
    ssd1306_WriteString("+ LwIP Init...", Font_6x8, White);
    ssd1306_UpdateScreen();
#endif

    (void)osKernelStart();
    Error_Handler();
    for (;;) {}
}
