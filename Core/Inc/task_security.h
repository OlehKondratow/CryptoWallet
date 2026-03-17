/**
  ******************************************************************************
  * @file    task_security.h
  * @brief   Security module - transaction decode, User Key (PC13) confirm, signing.
  ******************************************************************************
  * @details Decodes transactions (Recipient, Amount). Integrates with User Key
  *          for physical Confirm. Mock SHA256/signing (H7 HASH/PKA placeholders).
  *          Receives tx from task_net via g_tx_queue; User Confirmed from task_io.
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
