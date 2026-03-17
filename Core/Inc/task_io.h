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
 * @brief Create and start the IO task.
 * @note  Call from main() before vTaskStartScheduler().
 * @return None.
 */
void Task_IO_Create(void);

#ifdef __cplusplus
}
#endif

#endif /* __TASK_IO_H */
