/**
  ******************************************************************************
  * @file    task_security.c
  * @brief   Alternate signing FSM with **mock** SHA256/ECDSA (placeholders).
  ******************************************************************************
  * @details
  *          **Status:** Object file is linked, but @c Task_Security_Create() is **not**
  *          invoked from @c main.c . Production firmware uses @c task_sign.c with
  *          real crypto when @c USE_CRYPTO_SIGN . Keep this module for bring-up or
  *          comparison with Trezor-style FSM (IDLE→RECEIVED→WAIT_CONFIRM→…).
  *
  *          Uses @c memzero() for sensitive buffers; confirm timeout 30 s.
  *          **Docs:** @c docs_src/architecture.md (task table + data-flow).
  ******************************************************************************
  */

#include "task_security.h"
#include "main.h"
#include "wallet_shared.h"
#include "task_display.h"
#include "memzero.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"
#include <string.h>
#include <stdio.h>

#define SECURITY_STACK_SIZE      384U
#define SECURITY_PRIORITY        (tskIDLE_PRIORITY + 4)
#define SHA256_DIGEST_LEN        32U
#define SIG_LEN                  64U
#define CONFIRM_TIMEOUT_MS       30000U

/**
 * @brief Mock SHA256 - H7 HASH hardware acceleration placeholder.
 * @param data   Input buffer.
 * @param len    Input length in bytes.
 * @param digest Output 32-byte digest.
 * @return 0 on success, -1 on error.
 */
static int crypto_sha256(const uint8_t *data, size_t len, uint8_t digest[SHA256_DIGEST_LEN]);

/**
 * @brief Mock ECDSA sign - H7 PKA/CMOX hardware acceleration placeholder.
 * @param digest Input 32-byte SHA256 digest.
 * @param sig    Output 64-byte signature.
 * @return 0 on success, -1 on error.
 */
static int crypto_sign(const uint8_t digest[SHA256_DIGEST_LEN], uint8_t sig[SIG_LEN]);

/**
 * @brief Update display context with signing FSM state.
 * @param state Current signing state.
 * @param tx    Transaction (may be NULL).
 * @return None.
 */
static void display_update_signing(signing_state_t state, const wallet_tx_t *tx);

/**
 * @brief FSM handler: process one signing step.
 * @param state  Current state.
 * @param tx     Transaction (may be NULL).
 * @param digest SHA256 digest buffer.
 * @param sig    Signature output buffer.
 * @return Next signing state.
 */
static signing_state_t fsm_signing_step(signing_state_t state, wallet_tx_t *tx,
                                        uint8_t digest[SHA256_DIGEST_LEN],
                                        uint8_t sig[SIG_LEN]);

/**
 * @brief Security task entry - receives tx from queue, waits for User Confirm.
 * @param pvParameters  Unused.
 * @return None.
 */
static void security_task(void *pvParameters);

/**
 * @brief Create and start the security task.
 * @return None.
 */
void Task_Security_Create(void)
{
    xTaskCreate(security_task, "Security", SECURITY_STACK_SIZE, NULL,
                SECURITY_PRIORITY, NULL);
}

/**
 * @brief Mock SHA256 - replace with H7 HASH peripheral when available.
 */
static int crypto_sha256(const uint8_t *data, size_t len, uint8_t digest[SHA256_DIGEST_LEN])
{
    (void)data;
    (void)len;
    for (uint8_t i = 0; i < SHA256_DIGEST_LEN; i++) {
        digest[i] = (uint8_t)(0xA5 + i);
    }
    return 0;
}

/**
 * @brief Mock ECDSA sign - replace with H7 PKA/CMOX when available.
 */
static int crypto_sign(const uint8_t digest[SHA256_DIGEST_LEN], uint8_t sig[SIG_LEN])
{
    (void)digest;
    for (uint8_t i = 0; i < SIG_LEN; i++) {
        sig[i] = (uint8_t)(0xB7 + i);
    }
    return 0;
}

static void display_update_signing(signing_state_t state, const wallet_tx_t *tx)
{
    if (xSemaphoreTake(g_display_ctx_mutex, pdMS_TO_TICKS(50)) != pdTRUE) return;

    switch (state) {
        case SIGNING_IDLE:
            (void)strncpy(g_display_ctx.currency, "BTC", sizeof(g_display_ctx.currency) - 1);
            g_display_ctx.amount[0] = '\0';
            g_display_ctx.safe_locked = true;
            g_display_ctx.signature_valid = false;
            break;
        case SIGNING_RECEIVED:
        case SIGNING_WAIT_CONFIRM:
            if (tx != NULL) {
                (void)strncpy(g_display_ctx.currency, tx->currency[0] ? tx->currency : "BTC",
                              sizeof(g_display_ctx.currency) - 1);
                (void)strncpy(g_display_ctx.amount, tx->amount, sizeof(g_display_ctx.amount) - 1);
            }
            g_display_ctx.safe_locked = false;
            g_display_ctx.signature_valid = false;
            break;
        case SIGNING_DONE:
            g_display_ctx.safe_locked = true;
            g_display_ctx.signature_valid = true;
            UI_ClearPending();
            break;
        case SIGNING_REJECTED:
        case SIGNING_ERROR:
            g_display_ctx.safe_locked = true;
            g_display_ctx.signature_valid = false;
            UI_ClearPending();
            break;
        default:
            break;
    }
    xSemaphoreGive(g_display_ctx_mutex);
}

static signing_state_t fsm_signing_step(signing_state_t state, wallet_tx_t *tx,
                                        uint8_t digest[SHA256_DIGEST_LEN],
                                        uint8_t sig[SIG_LEN])
{
    switch (state) {
        case SIGNING_RECEIVED:
            xEventGroupClearBits(g_user_event_group, EVENT_USER_CONFIRMED | EVENT_USER_REJECTED);
            Task_Display_Log("TX recv");
            if (tx != NULL) Task_Display_Log(tx->recipient);
            if (crypto_sha256((const uint8_t *)tx->recipient, strlen(tx->recipient), digest) != 0) {
                Task_Display_Log("SHA256 err");
                g_security_alert = 1;
                return SIGNING_ERROR;
            }
            return SIGNING_WAIT_CONFIRM;

        case SIGNING_WAIT_CONFIRM: {
            EventBits_t bits = xEventGroupWaitBits(g_user_event_group,
                    EVENT_USER_CONFIRMED | EVENT_USER_REJECTED,
                    pdTRUE, pdFALSE, pdMS_TO_TICKS(CONFIRM_TIMEOUT_MS));
            if (bits & EVENT_USER_REJECTED) {
                Task_Display_Log("Rejected");
                memzero(digest, SHA256_DIGEST_LEN);
                return SIGNING_REJECTED;
            }
            if (!(bits & EVENT_USER_CONFIRMED)) {
                Task_Display_Log("Timeout");
                memzero(digest, SHA256_DIGEST_LEN);
                return SIGNING_REJECTED;
            }
            return SIGNING_IN_PROGRESS;
        }

        case SIGNING_IN_PROGRESS:
            if (crypto_sign(digest, sig) != 0) {
                Task_Display_Log("Sign err");
                g_security_alert = 1;
                memzero(digest, SHA256_DIGEST_LEN);
                memzero(sig, SIG_LEN);
                return SIGNING_ERROR;
            }
            memzero(digest, SHA256_DIGEST_LEN);
            memzero(sig, SIG_LEN);
            Task_Display_Log("Signed OK");
            g_security_alert = 0;
            return SIGNING_DONE;

        default:
            return state;
    }
}

static void security_task(void *pvParameters)
{
    (void)pvParameters;
    signing_state_t state = SIGNING_IDLE;
    wallet_tx_t tx;
    uint8_t digest[SHA256_DIGEST_LEN];
    uint8_t sig[SIG_LEN];

    memzero(&tx, sizeof(tx));
    memzero(digest, sizeof(digest));
    memzero(sig, sizeof(sig));

    display_update_signing(SIGNING_IDLE, NULL);
    Task_Display_Log("Security init");

    for (;;) {
        if (state == SIGNING_IDLE) {
            if (xQueueReceive(g_tx_queue, &tx, pdMS_TO_TICKS(200)) != pdTRUE) {
                continue;
            }
            state = SIGNING_RECEIVED;
            display_update_signing(state, &tx);
        }

        state = fsm_signing_step(state, &tx, digest, sig);
        display_update_signing(state, &tx);

        if (state == SIGNING_DONE || state == SIGNING_REJECTED || state == SIGNING_ERROR) {
            memzero(&tx, sizeof(tx));
            state = SIGNING_IDLE;
        }
    }
}
