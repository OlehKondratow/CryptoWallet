/**
  ******************************************************************************
  * @file    task_sign.c
  * @brief   Signing task — request analysis, validation, ECDSA secp256k1.
  ******************************************************************************
  * @details Original implementation. Validates incoming tx via tx_request_validate,
  *          hashes with SHA-256, waits for user confirm (g_user_event_group),
  *          derives key m/44'/0'/0'/0/0, signs with crypto_sign_btc_hash.
  *          No code copied from trezor-firmware.
  ******************************************************************************
  */

#include "task_sign.h"
#include "main.h"
#include "wallet_shared.h"
#include "task_display.h"
#include "tx_request_validate.h"
#include "crypto_wallet.h"
#include "memzero.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"
#include <string.h>
#include <stdio.h>

#define SIGN_STACK_SIZE      512U
#define SIGN_PRIORITY        (tskIDLE_PRIORITY + 4)
#define CONFIRM_TIMEOUT_MS   30000U

/**
 * @brief Provide wallet seed for key derivation.
 * @param seed_out Output buffer for 64-byte BIP-39 seed.
 * @param max_len  Buffer size (must be >= 64).
 * @return 0 on success, -1 if seed not available.
 * @note Implement in secure storage module. Weak stub returns -1.
 */
__attribute__((weak)) int get_wallet_seed(uint8_t *seed_out, size_t max_len);

static void sign_task(void *pvParameters);

void Task_Sign_Create(void)
{
    xTaskCreate(sign_task, "Sign", SIGN_STACK_SIZE, NULL, SIGN_PRIORITY, NULL);
}

__attribute__((weak)) int get_wallet_seed(uint8_t *seed_out, size_t max_len)
{
    (void)seed_out;
    (void)max_len;
    return -1;  /* No seed — implement in secure storage */
}

/**
 * @brief Build data to hash: recipient|amount|currency (deterministic).
 */
static size_t build_hash_input(const wallet_tx_t *tx, char *buf, size_t buf_size)
{
    if (tx == NULL || buf == NULL || buf_size == 0) return 0;

    const char *curr = (tx->currency[0] != '\0') ? tx->currency : "BTC";
    int n = snprintf(buf, buf_size, "%s|%s|%s", tx->recipient, tx->amount, curr);
    if (n < 0 || (size_t)n >= buf_size) return 0;
    return (size_t)n;
}

static void sign_task(void *pvParameters)
{
    (void)pvParameters;
    wallet_tx_t tx;
    uint8_t digest[CRYPTO_SHA256_DIGEST_LEN];
    uint8_t priv_key[32];
    uint8_t sig[CRYPTO_ECDSA_SIG_LEN];
    char hash_input[TX_RECIPIENT_LEN + TX_AMOUNT_LEN + TX_CURRENCY_LEN + 4];
    signing_state_t state = SIGNING_IDLE;

    memzero(&tx, sizeof(tx));
    memzero(digest, sizeof(digest));
    memzero(priv_key, sizeof(priv_key));
    memzero(sig, sizeof(sig));
    memset(hash_input, 0, sizeof(hash_input));

    if (xSemaphoreTake(g_display_ctx_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        (void)strncpy(g_display_ctx.currency, "BTC", sizeof(g_display_ctx.currency) - 1);
        g_display_ctx.amount[0] = '\0';
        g_display_ctx.safe_locked = true;
        g_display_ctx.signature_valid = false;
        xSemaphoreGive(g_display_ctx_mutex);
    }
    Task_Display_Log("Sign task init");

    for (;;) {
        if (state == SIGNING_IDLE) {
            if (xQueueReceive(g_tx_queue, &tx, pdMS_TO_TICKS(200)) != pdTRUE) {
                continue;
            }

            /* --- Request analysis & validation --- */
            tx_validate_result_t vr = tx_request_validate(&tx);
            if (vr != TX_VALID_OK) {
                Task_Display_Log(tx_validate_result_str(vr));
                g_security_alert = 1;
                memzero(&tx, sizeof(tx));
                continue;
            }

            state = SIGNING_RECEIVED;
        }

        /* Update display */
        if (xSemaphoreTake(g_display_ctx_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            (void)strncpy(g_display_ctx.currency, tx.currency[0] ? tx.currency : "BTC",
                         sizeof(g_display_ctx.currency) - 1);
            (void)strncpy(g_display_ctx.amount, tx.amount, sizeof(g_display_ctx.amount) - 1);
            g_display_ctx.safe_locked = false;
            g_display_ctx.signature_valid = false;
            xSemaphoreGive(g_display_ctx_mutex);
        }

        if (state == SIGNING_RECEIVED) {
            Task_Display_Log("TX recv");
            Task_Display_Log(tx.recipient);

            size_t len = build_hash_input(&tx, hash_input, sizeof(hash_input));
            if (len == 0 || crypto_hash_sha256((const uint8_t *)hash_input, len, digest) != 0) {
                Task_Display_Log("Hash err");
                g_security_alert = 1;
                memzero(&tx, sizeof(tx));
                memzero(digest, sizeof(digest));
                state = SIGNING_IDLE;
                continue;
            }
            state = SIGNING_WAIT_CONFIRM;
        }

        if (state == SIGNING_WAIT_CONFIRM) {
            xEventGroupClearBits(g_user_event_group, EVENT_USER_CONFIRMED | EVENT_USER_REJECTED);
            EventBits_t bits = xEventGroupWaitBits(g_user_event_group,
                    EVENT_USER_CONFIRMED | EVENT_USER_REJECTED,
                    pdTRUE, pdFALSE, pdMS_TO_TICKS(CONFIRM_TIMEOUT_MS));

            if (bits & EVENT_USER_REJECTED) {
                Task_Display_Log("Rejected");
                memzero(digest, sizeof(digest));
                memzero(&tx, sizeof(tx));
                state = SIGNING_IDLE;
                continue;
            }
            if (!(bits & EVENT_USER_CONFIRMED)) {
                Task_Display_Log("Timeout");
                memzero(digest, sizeof(digest));
                memzero(&tx, sizeof(tx));
                state = SIGNING_IDLE;
                continue;
            }

            /* Get seed and derive key m/44'/0'/0'/0/0 */
            uint8_t seed[64];
            memzero(seed, sizeof(seed));
            if (get_wallet_seed(seed, sizeof(seed)) != 0) {
                Task_Display_Log("No seed");
                g_security_alert = 1;
                memzero(digest, sizeof(digest));
                memzero(&tx, sizeof(tx));
                state = SIGNING_IDLE;
                continue;
            }
            if (crypto_derive_btc_m44_0_0_0_0(seed, 64, priv_key) != 0) {
                memzero(seed, sizeof(seed));
                Task_Display_Log("Derive err");
                g_security_alert = 1;
                memzero(digest, sizeof(digest));
                memzero(&tx, sizeof(tx));
                state = SIGNING_IDLE;
                continue;
            }
            memzero(seed, sizeof(seed));

            /* Sign */
            if (crypto_sign_btc_hash(priv_key, digest, sig) != 0) {
                Task_Display_Log("Sign err");
                g_security_alert = 1;
                memzero(digest, sizeof(digest));
                memzero(sig, sizeof(sig));
                memzero(&tx, sizeof(tx));
                state = SIGNING_IDLE;
                continue;
            }

            memcpy(g_last_sig, sig, CRYPTO_ECDSA_SIG_LEN);
            g_last_sig_ready = 1;
#if defined(USE_WEBUSB) && (USE_WEBUSB == 1)
            extern void WebUSB_NotifySignatureReady(void);
            WebUSB_NotifySignatureReady();
#endif
            memzero(digest, sizeof(digest));
            memzero(sig, sizeof(sig));
            memzero(&tx, sizeof(tx));
            g_security_alert = 0;
            Task_Display_Log("Signed OK");

            if (xSemaphoreTake(g_display_ctx_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
                g_display_ctx.safe_locked = true;
                g_display_ctx.signature_valid = true;
                UI_ClearPending();
                xSemaphoreGive(g_display_ctx_mutex);
            }
            state = SIGNING_IDLE;
        }
    }
}
