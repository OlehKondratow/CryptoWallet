/**
  ******************************************************************************
  * @file    task_io.c
  * @brief   IO module - LEDs and User Button with software debouncing.
  ******************************************************************************
  * @details LED1 (Green) System OK, LED2 (Yellow) Network, LED3 (Red) Security.
  *          User Button PC13: short press = Confirm, long press (2.5s) = Reject.
  *          Debounce 50ms. Signals task_security via g_user_event_group.
  ******************************************************************************
  */

#include "task_io.h"
#include "main.h"
#include "wallet_shared.h"
#include "task_display.h"
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"
#include <stdbool.h>

#define IO_STACK_SIZE        256U
#define IO_PRIORITY          (tskIDLE_PRIORITY + 1)
#define DEBOUNCE_MS          50U
#define LONG_PRESS_MS        2500U
#define POLL_MS              20U

static void io_task(void *pvParameters);

/**
 * @brief Set LED1 (Green - System OK).
 * @param on  true = ON, false = OFF.
 */
static void led1_set(bool on);

/**
 * @brief Set LED2 (Yellow - Network).
 * @param on  true = ON, false = OFF.
 */
static void led2_set(bool on);

/**
 * @brief Set LED3 (Red - Security Alert).
 * @param on  true = ON, false = OFF.
 */
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

/**
 * @brief IO task entry - poll User Button, update LEDs.
 * @param pvParameters  Unused.
 * @return None.
 */
static void io_task(void *pvParameters)
{
    (void)pvParameters;
    bool btn_last = false;
    uint32_t btn_stable_ticks = 0;
    uint32_t last_press_duration = 0;
    bool action_fired = false;
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

        GPIO_PinState pin = HAL_GPIO_ReadPin(USER_KEY_GPIO_PORT, USER_KEY_PIN);
        bool btn_pressed = (pin == USER_KEY_PRESSED);

        if (btn_pressed == btn_last) {
            btn_stable_ticks += POLL_MS;
        } else {
            if (!btn_pressed) {
                action_fired = false;
            }
            btn_stable_ticks = 0;
        }
        btn_last = btn_pressed;

        if (btn_pressed) {
            last_press_duration = btn_stable_ticks;
            if (last_press_duration >= LONG_PRESS_MS && !action_fired) {
                action_fired = true;
                xEventGroupSetBits(g_user_event_group, EVENT_USER_REJECTED);
                Task_Display_Log("Reject");
            }
        } else {
            /* Just released: short press = confirm */
            if (last_press_duration >= DEBOUNCE_MS && last_press_duration < LONG_PRESS_MS && !action_fired) {
                action_fired = true;
                xEventGroupSetBits(g_user_event_group, EVENT_USER_CONFIRMED);
                Task_Display_Log("Confirm");
            }
            last_press_duration = 0;
        }

        led1_set(true);
#ifndef USE_LWIP
        led2_set((tick_count / 25) % 2 == 0);
#endif
        led3_set(g_security_alert != 0);
    }
}
