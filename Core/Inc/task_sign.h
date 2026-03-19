/**
  ******************************************************************************
  * @file    task_sign.h
  * @brief   Production signing task — @c g_tx_queue consumer, USER confirm, ECDSA.
  ******************************************************************************
  * @details
  *          **Active path in @c main.c** (@c Task_Sign_Create ). Pulls @c wallet_tx_t ,
  *          validates via @c tx_request_validate , hashes, waits on
  *          @c g_user_event_group , signs with @c crypto_wallet / trezor-crypto.
  *          Original wallet logic; not copied from trezor-firmware.
  ******************************************************************************
  */

#ifndef __TASK_SIGN_H__
#define __TASK_SIGN_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Start the @c "Sign" FreeRTOS task.
 * @details Depends on @c g_tx_queue , @c g_user_event_group , @c g_display_ctx_mutex ,
 *          @c crypto_rng_init() when @c USE_CRYPTO_SIGN . Weak @c get_wallet_seed() must be
 *          overridden for non-test builds.
 */
void Task_Sign_Create(void);

#ifdef __cplusplus
}
#endif

#endif /* __TASK_SIGN_H__ */
