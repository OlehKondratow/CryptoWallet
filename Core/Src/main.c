/**
  ******************************************************************************
  * @file    main.c
  * @brief   FreeRTOS entry: IPC objects, task creation, OS hooks.
  ******************************************************************************
  * @details
  *          **Product (high-level):** Embedded wallet on STM32H743 (Nucleo-144): receive
  *          sign requests over Ethernet (HTTP) and/or WebUSB, confirm on USER button,
  *          show status on SSD1306; optional full ECDSA via trezor-crypto (@c USE_CRYPTO_SIGN ).
  *
  *          **Tasks started here:** @c Task_Display_Create , @c Task_Net_Create ,
  *          @c Task_Sign_Create , @c Task_IO_Create , @c Task_User_Create .
  *          @c task_security.c is linked, but @c Task_Security_Create() is **not** called
  *          from this file — production signing runs in @c task_sign.c .
  *
  *          **IPC (globals by design):** @c g_tx_queue (net → sign), @c g_display_queue ,
  *          @c g_user_event_group (user button → sign), I2C/UI/display mutexes — see
  *          @c wallet_shared.h .
  *
  *          **Boot order:** with @c USE_LWIP , @c HW_Init_Early_LwIP() before @c HAL_Init()
  *          (MPU/cache, lwip_zero order). Then @c HW_Init() , @c time_service_init() ,
  *          @c crypto_rng_init() when crypto enabled.
  *
  *          **Build flags:** @c BOOT_TEST — no FreeRTOS; @c SKIP_OLED — skip OLED path.
  *          **Hooks:** @c vApplicationMallocFailedHook , @c vApplicationStackOverflowHook ,
  *          @c Error_Handler .
  *
  *          See @c docs_src/architecture.md , repository @c README.md .
  ******************************************************************************
  */

#include "main.h"
#include "hw_init.h"
#include "time_service.h"
#include "wallet_shared.h"
#include "task_display.h"
#include "task_net.h"
#include "task_sign.h"
#include "task_io.h"
#include "task_user.h"
#ifdef USE_CRYPTO_SIGN
#include "crypto_wallet.h"
#endif
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
uint8_t          g_last_sig[64] = {0};
volatile uint8_t  g_last_sig_ready = 0;

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
#ifdef USE_CRYPTO_SIGN
    crypto_rng_init();
#endif
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
    Task_Sign_Create();
    Task_IO_Create();
    Task_User_Create();
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
