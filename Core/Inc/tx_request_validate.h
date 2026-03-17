/**
  ******************************************************************************
  * @file    tx_request_validate.h
  * @brief   Request analysis and validation for crypto transaction signing.
  ******************************************************************************
  * @details Original implementation — no code copied from trezor-firmware.
  *          Validates recipient format, amount, currency before signing.
  ******************************************************************************
  */

#ifndef __TX_REQUEST_VALIDATE_H__
#define __TX_REQUEST_VALIDATE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "wallet_shared.h"

/**
 * @brief Validation result codes.
 */
typedef enum {
    TX_VALID_OK = 0,
    TX_VALID_ERR_RECIPIENT_EMPTY,
    TX_VALID_ERR_RECIPIENT_TOO_LONG,
    TX_VALID_ERR_RECIPIENT_INVALID_CHARS,
    TX_VALID_ERR_RECIPIENT_BAD_PREFIX,
    TX_VALID_ERR_AMOUNT_EMPTY,
    TX_VALID_ERR_AMOUNT_INVALID,
    TX_VALID_ERR_AMOUNT_NEGATIVE,
    TX_VALID_ERR_AMOUNT_OVERFLOW,
    TX_VALID_ERR_CURRENCY_UNSUPPORTED,
    TX_VALID_ERR_NULL_PTR
} tx_validate_result_t;

/**
 * @brief Validate transaction request before signing.
 * @param tx Pointer to transaction (recipient, amount, currency).
 * @return TX_VALID_OK on success, error code otherwise.
 * @note Call before enqueueing to g_tx_queue or before signing.
 */
tx_validate_result_t tx_request_validate(const wallet_tx_t *tx);

/**
 * @brief Get human-readable error string for validation result.
 * @param result Validation result code.
 * @return Static string describing the error.
 */
const char *tx_validate_result_str(tx_validate_result_t result);

/**
 * @brief Check if recipient looks like a valid Bitcoin address (P2PKH/P2SH).
 * @param recipient Null-terminated string.
 * @return true if format appears valid.
 * @note Basic check: length 26-35, base58 chars, prefix 1 or 3.
 */
bool tx_recipient_format_ok(const char *recipient);

/**
 * @brief Check if amount string is valid and positive.
 * @param amount Null-terminated string (e.g. "0.001").
 * @return true if parseable and >= 0.
 */
bool tx_amount_format_ok(const char *amount);

/**
 * @brief Check if currency is in supported whitelist.
 * @param currency Null-terminated string (e.g. "BTC", "ETH").
 * @return true if supported.
 */
bool tx_currency_supported(const char *currency);

#ifdef __cplusplus
}
#endif

#endif /* __TX_REQUEST_VALIDATE_H__ */
