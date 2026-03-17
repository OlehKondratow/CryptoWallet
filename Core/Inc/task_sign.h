/**
  ******************************************************************************
  * @file    task_sign.h
  * @brief   Signing task — request analysis, validation, ECDSA signing.
  ******************************************************************************
  * @details Original implementation. Receives tx from g_tx_queue, validates
  *          via tx_request_validate, hashes, waits for user confirm, signs
  *          with crypto_wallet (trezor-crypto). No code copied from trezor-firmware.
  ******************************************************************************
  */

#ifndef __TASK_SIGN_H__
#define __TASK_SIGN_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create and start the signing task.
 * @note  Call from main() before vTaskStartScheduler().
 *        Requires: g_tx_queue, g_user_event_group, g_display_ctx_mutex.
 *        Implement get_wallet_seed() for seed retrieval (secure storage).
 * @return None.
 */
void Task_Sign_Create(void);

#ifdef __cplusplus
}
#endif

#endif /* __TASK_SIGN_H__ */
