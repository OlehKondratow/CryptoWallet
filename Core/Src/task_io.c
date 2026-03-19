/**
  ******************************************************************************
  * @file    task_io.c
  * @brief   LED policy task — system / network / alert indicators only.
  ******************************************************************************
  * @details
  *          **Scope:** LED1 green (alive), LED2 yellow (network hint), LED3 red
  *          (@c g_security_alert ). **Does not** read USER button — that is
  *          @c task_user.c → @c g_user_event_group → @c task_sign.c .
  *
  *          Matches “one module, one responsibility” from architecture review.
  *          **Pins:** @c main.h .
  ******************************************************************************
  */

#include "task_io.h"
#include "main.h"
#include "wallet_shared.h"
#include "task_display.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdbool.h>

#define IO_STACK_SIZE        128U
#define IO_PRIORITY          (tskIDLE_PRIORITY + 1)
#define POLL_MS              100U

/** @brief LED update loop; reflects @c g_security_alert on LED3. */
static void io_task(void *pvParameters);

/** @brief Drive LED1 (green) active level per board polarity in @c main.h . */
static void led1_set(bool on);
/** @brief Drive LED2 (yellow). */
static void led2_set(bool on);
/** @brief Drive LED3 (red) — security alert. */
static void led3_set(bool on);

void Task_IO_Create(void)
{
    xTaskCreate(io_task, "IO", IO_STACK_SIZE, NULL, IO_PRIORITY, NULL);
}

static void led1_set(bool on)
{
    HAL_GPIO_WritePin(LED1_GPIO_PORT, LED1_PIN, on ? LED1_ON_LEVEL : LED1_OFF_LEVEL);
}

static void led2_set(bool on)
{
    HAL_GPIO_WritePin(LED2_GPIO_PORT, LED2_PIN, on ? LED2_ON_LEVEL : LED2_OFF_LEVEL);
}

static void led3_set(bool on)
{
    HAL_GPIO_WritePin(LED3_GPIO_PORT, LED3_PIN, on ? LED3_ON_LEVEL : LED3_OFF_LEVEL);
}

static void io_task(void *pvParameters)
{
    (void)pvParameters;
    uint32_t tick_count = 0;

    led1_set(true);
#ifndef USE_LWIP
    led2_set(false);
#endif
    led3_set(false);

    Task_Display_Log("IO init");

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(POLL_MS));
        tick_count++;

        led1_set(true);
#ifndef USE_LWIP
        led2_set((tick_count / 25) % 2 == 0);
#endif
        led3_set(g_security_alert != 0);
    }
}
