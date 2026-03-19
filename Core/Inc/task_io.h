/**
  ******************************************************************************
  * @file    task_io.h
  * @brief   IO module - LEDs only.
  ******************************************************************************
  * @details LED1 (Green) System OK, LED2 (Yellow) Network, LED3 (Red) Security.
  *          USER button handling in task_user.c.
  ******************************************************************************
  */

#ifndef __TASK_IO_H
#define __TASK_IO_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Create LED “heartbeat / alert” task (no button handling).
 * @details Periodically updates LED1 always-on, LED2 toggle when @c USE_LWIP is off,
 *          LED3 from @c g_security_alert . Stack 128, priority idle+1.
 */
void Task_IO_Create(void);

#ifdef __cplusplus
}
#endif

#endif /* __TASK_IO_H */
