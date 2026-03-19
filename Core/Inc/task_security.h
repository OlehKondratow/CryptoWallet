/**
  ******************************************************************************
  * @file    task_security.h
  * @brief   Header for legacy @c task_security.c (mock crypto FSM).
  ******************************************************************************
  * @details
  *          **Not started from @c main.c** — see @c task_sign.c for the queue consumer
  *          fed by @c task_net . USER events come from @c task_user.c via
  *          @c g_user_event_group . Mock SHA256/ECDSA here vs real trezor path in sign task.
  ******************************************************************************
  */

#ifndef __TASK_SECURITY_H
#define __TASK_SECURITY_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create and start the security task.
 * @note  Call from main() before vTaskStartScheduler().
 * @return None.
 */
void Task_Security_Create(void);

#ifdef __cplusplus
}
#endif

#endif /* __TASK_SECURITY_H */
