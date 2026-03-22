/**
  ******************************************************************************
  * @file    task_user.c
  * @brief   Physical UX — USER (PC13) debounce, confirm vs reject for signing.
  ******************************************************************************
  * @details
  *          **Architecture:** Sole owner of the USER GPIO for wallet policy; isolates
  *          button timing from @c task_io.c (LEDs only). Short press →
  *          @c EVENT_USER_CONFIRMED ; ~2.5 s hold → @c EVENT_USER_REJECTED .
  *          Debounce 50 ms; @c task_sign.c waits on @c g_user_event_group .
  *
  *          **Docs:** @c docs_src/testing-signing.md , @c docs_src/architecture.md .
  ******************************************************************************
  */

#include "task_user.h"
#include "main.h"
#include "wallet_shared.h"
#include "task_display.h"
#include "app_log.h"
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"
#include <stdbool.h>

#define USER_STACK_SIZE      192U
#define USER_PRIORITY        (tskIDLE_PRIORITY + 2)
#define DEBOUNCE_MS          50U
#define LONG_PRESS_MS        2500U
#define POLL_MS              20U

/** @brief Poll USER GPIO, debounce, set @c g_user_event_group bits. */
static void user_task(void *pvParameters);

void Task_User_Create(void)
{
    xTaskCreate(user_task, "User", USER_STACK_SIZE, NULL, USER_PRIORITY, NULL);
}

static void user_task(void *pvParameters)
{
    (void)pvParameters;
    bool btn_last = false;
    uint32_t btn_stable_ticks = 0;
    uint32_t last_press_duration = 0;
    bool action_fired = false;

    APP_LOG_INFO("[USER] task started");
    APP_LOG_INFO("[USER] button debounce active");
    APP_LOG_WALLET_SUB_INFO("USER");

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(POLL_MS));

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
                APP_LOG_INFO("[USER] confirm");
            }
            last_press_duration = 0;
        }
    }
}
