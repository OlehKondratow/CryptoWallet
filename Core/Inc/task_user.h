/**
  ******************************************************************************
  * @file    task_user.h
  * @brief   User button task — USER key (PC13) handling.
  ******************************************************************************
  * @details Dedicated task for USER button. Debounce 50ms. Short press = Confirm,
  *          long press (2.5s) = Reject. Signals task_sign via g_user_event_group.
  ******************************************************************************
  */

#ifndef __TASK_USER_H__
#define __TASK_USER_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Create FreeRTOS task that polls USER button (PC13) with debounce.
 * @details Priority @c tskIDLE_PRIORITY+2 , stack 192 words. Sets @c EVENT_USER_CONFIRMED or
 *          @c EVENT_USER_REJECTED on @c g_user_event_group . Call before @c vTaskStartScheduler() .
 */
void Task_User_Create(void);

#ifdef __cplusplus
}
#endif

#endif /* __TASK_USER_H__ */
