/**
  ******************************************************************************
  * @file    tx_request_validate.h
  * @brief   Request analysis and validation for crypto transaction signing.
  ******************************************************************************
  * @details
  *          Original implementation — not copied from trezor-firmware. All host-supplied
  *          strings are checked before they reach @c g_tx_queue or signing. Reduces risk of
  *          malformed UI and obviously invalid addresses. **Not** a substitute for full
  *          on-chain transaction validation.
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
 * @brief Machine-readable result of @c tx_request_validate() .
 * @details Each value maps to a static string via @c tx_validate_result_str() for logging/UI.
 */
typedef enum {
    TX_VALID_OK = 0,                        /**< All checks passed. */
    TX_VALID_ERR_RECIPIENT_EMPTY,           /**< @c recipient[0] == '\\0' . */
    TX_VALID_ERR_RECIPIENT_TOO_LONG,        /**< Length >= @c TX_RECIPIENT_LEN . */
    TX_VALID_ERR_RECIPIENT_INVALID_CHARS,   /**< Fails @c tx_recipient_format_ok() . */
    TX_VALID_ERR_RECIPIENT_BAD_PREFIX,      /**< Reserved / unused in current implementation. */
    TX_VALID_ERR_AMOUNT_EMPTY,              /**< @c amount[0] == '\\0' . */
    TX_VALID_ERR_AMOUNT_INVALID,            /**< Not a sane decimal string (see @c tx_amount_format_ok ). */
    TX_VALID_ERR_AMOUNT_NEGATIVE,           /**< Parsed @c atof(amount) < 0 . */
    TX_VALID_ERR_AMOUNT_OVERFLOW,           /**< Reserved for future range checks. */
    TX_VALID_ERR_CURRENCY_UNSUPPORTED,    /**< Not in internal whitelist (see @c tx_currency_supported ). */
    TX_VALID_ERR_NULL_PTR                   /**< @c tx == NULL . */
} tx_validate_result_t;

/**
 * @brief   Full validation gate for a @c wallet_tx_t before signing.
 * @details Runs null checks, length checks, @c tx_recipient_format_ok , @c tx_amount_format_ok ,
 *          positive amount, and @c tx_currency_supported (defaults empty currency to "BTC" path
 *          inside implementation). Call from HTTP handler, WebUSB, and optionally again in @c task_sign.c .
 * @param   tx  Pointer to populated transaction; must remain valid for the call duration.
 * @return  @c TX_VALID_OK or the first failing error code.
 */
tx_validate_result_t tx_request_validate(const wallet_tx_t *tx);

/**
 * @brief   Map validation code to a short English phrase for logs and display.
 * @details Returns pointer to static storage; do not free. Safe from any task after init.
 * @param   result  Value returned by @c tx_request_validate() .
 * @return  Human-readable ASCII string; @c "unknown" for out-of-range enum values.
 */
const char *tx_validate_result_str(tx_validate_result_t result);

/**
 * @brief   Heuristic check for Bitcoin-style address strings (P2PKH/P2SH/bech32 subset).
 * @details
 *          - Legacy: length 26–35, prefix @c '1' or @c '3' , Base58 alphabet only.
 *          - Bech32: prefix @c "bc1" , length rules and charset per simplified embedded check.
 *          Does **not** verify checksum or network; only format.
 * @param   recipient  Null-terminated string from host.
 * @return  @c true if format passes heuristics; @c false otherwise.
 */
bool tx_recipient_format_ok(const char *recipient);

/**
 * @brief   Validate amount as a non-empty decimal string with at least one digit.
 * @details Allows optional leading @c '+' ; rejects leading @c '-' . Single @c '.' allowed.
 *          Does not cap maximum magnitude (see @c TX_VALID_ERR_AMOUNT_OVERFLOW for future use).
 * @param   amount  Null-terminated string (e.g. @c "0.001" ).
 * @return  @c true if parseable pattern is valid and non-negative form is used.
 */
bool tx_amount_format_ok(const char *amount);

/**
 * @brief   Case-insensitive match against fixed whitelist (BTC, ETH, LTC, …).
 * @details List is compiled into @c tx_request_validate.c ; extend there for new assets.
 * @param   currency  Null-terminated ticker (e.g. @c "BTC" ).
 * @return  @c true if supported.
 */
bool tx_currency_supported(const char *currency);

#ifdef __cplusplus
}
#endif

#endif /* __TX_REQUEST_VALIDATE_H__ */
