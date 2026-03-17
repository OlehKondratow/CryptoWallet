/**
  ******************************************************************************
  * @file    task_io.h
  * @brief   IO module - LEDs and User Button (PC13) with software debouncing.
  ******************************************************************************
  * @details LED1 (Green) System OK, LED2 (Yellow) Network, LED3 (Red) Security.
  *          User Button: short press = Confirm, long press = Reject.
  *          Signals task_security via Event Group (EVENT_USER_CONFIRMED/REJECTED).
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
