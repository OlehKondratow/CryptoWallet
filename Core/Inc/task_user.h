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
 * @brief Create and start the USER button task.
 * @note  Call from main() before vTaskStartScheduler().
 *        Requires: g_user_event_group.
 * @return None.
 */
void Task_User_Create(void);

#ifdef __cplusplus
}
#endif

#endif /* __TASK_USER_H__ */
