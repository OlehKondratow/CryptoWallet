/**
  ******************************************************************************
  * @file    tx_request_validate.c
  * @brief   Validate host-supplied recipient / amount / currency before signing.
  ******************************************************************************
  * @details
  *          Whitelist currencies, Base58 checks for BTC-style addresses, numeric
  *          amount rules. Used from task_net.c (HTTP), usb_webusb.c (WebUSB), and
  *          task_sign.c. **Messages:** documentation/03-cryptography-and-signing.md .
  ******************************************************************************
  */

#include "tx_request_validate.h"
#include "wallet_shared.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

/** @brief Case-insensitive ASCII equality for currency tickers. */
static int str_eq_ignore_case(const char *a, const char *b)
{
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return 0;
        a++; b++;
    }
    return (*a == '\0' && *b == '\0');
}

/* Supported currencies whitelist */
static const char * const SUPPORTED_CURRENCIES[] = {
    "BTC", "ETH", "LTC", "BCH", "DOGE", "DASH", "XMR"
};
#define SUPPORTED_COUNT (sizeof(SUPPORTED_CURRENCIES) / sizeof(SUPPORTED_CURRENCIES[0]))

/** @brief Return true if @a c is in Bitcoin Base58 alphabet (excludes 0,O,I,l). */
static bool is_base58_char(char c)
{
    if (c >= '1' && c <= '9') return true;
    if (c >= 'A' && c <= 'H') return true;
    if (c >= 'J' && c <= 'N') return true;
    if (c >= 'P' && c <= 'Z') return true;
    if (c >= 'a' && c <= 'k') return true;
    if (c >= 'm' && c <= 'z') return true;
    return false;
}

/** @copydoc tx_recipient_format_ok */
bool tx_recipient_format_ok(const char *recipient)
{
    if (recipient == NULL) return false;

    size_t len = strlen(recipient);
    /* P2PKH/P2SH: 26-35 chars; bech32: 42-62 */
    if (len < 26 || len > 62) return false;

    /* P2PKH: 1...; P2SH: 3...; bech32: bc1... */
    if (recipient[0] == '1' || recipient[0] == '3') {
        if (len > 35) return false;
        for (size_t i = 0; i < len; i++) {
            if (!is_base58_char(recipient[i])) return false;
        }
    } else if (len >= 4 && strncmp(recipient, "bc1", 3) == 0) {
        /* bech32: alphanumeric except b,i,o */
        for (size_t i = 4; i < len; i++) {
            char c = recipient[i];
            if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z' && c != 'b' && c != 'i' && c != 'o'))
                continue;
            return false;
        }
    } else {
        return false;
    }
    return true;
}

/** @copydoc tx_amount_format_ok */
bool tx_amount_format_ok(const char *amount)
{
    if (amount == NULL || amount[0] == '\0') return false;

    bool has_dot = false;
    bool has_digit = false;
    for (const char *p = amount; *p != '\0'; p++) {
        if (*p == '.') {
            if (has_dot) return false;
            has_dot = true;
        } else if (isdigit((unsigned char)*p)) {
            has_digit = true;
        } else if (*p == '-' || *p == '+') {
            if (p != amount) return false;
            if (*p == '-') return false; /* No negative amounts */
        } else {
            return false;
        }
    }
    return has_digit;
}

bool tx_currency_supported(const char *currency)
{
    if (currency == NULL || currency[0] == '\0') return false;

    for (size_t i = 0; i < SUPPORTED_COUNT; i++) {
        if (str_eq_ignore_case(currency, SUPPORTED_CURRENCIES[i]))
            return true;
    }
    return false;
}

/** @copydoc tx_request_validate */
tx_validate_result_t tx_request_validate(const wallet_tx_t *tx)
{
    if (tx == NULL) return TX_VALID_ERR_NULL_PTR;

    /* Recipient */
    if (tx->recipient[0] == '\0') return TX_VALID_ERR_RECIPIENT_EMPTY;
    if (strlen(tx->recipient) >= TX_RECIPIENT_LEN) return TX_VALID_ERR_RECIPIENT_TOO_LONG;
    if (!tx_recipient_format_ok(tx->recipient)) return TX_VALID_ERR_RECIPIENT_INVALID_CHARS;

    /* Amount */
    if (tx->amount[0] == '\0') return TX_VALID_ERR_AMOUNT_EMPTY;
    if (!tx_amount_format_ok(tx->amount)) return TX_VALID_ERR_AMOUNT_INVALID;
    double amt = atof(tx->amount);
    if (amt < 0.0) return TX_VALID_ERR_AMOUNT_NEGATIVE;

    /* Currency */
    const char *curr = (tx->currency[0] != '\0') ? tx->currency : "BTC";
    if (!tx_currency_supported(curr)) return TX_VALID_ERR_CURRENCY_UNSUPPORTED;

    return TX_VALID_OK;
}

/** @copydoc tx_validate_result_str */
const char *tx_validate_result_str(tx_validate_result_t result)
{
    switch (result) {
        case TX_VALID_OK:                    return "OK";
        case TX_VALID_ERR_RECIPIENT_EMPTY:   return "recipient empty";
        case TX_VALID_ERR_RECIPIENT_TOO_LONG: return "recipient too long";
        case TX_VALID_ERR_RECIPIENT_INVALID_CHARS: return "recipient invalid";
        case TX_VALID_ERR_RECIPIENT_BAD_PREFIX:   return "recipient bad prefix";
        case TX_VALID_ERR_AMOUNT_EMPTY:      return "amount empty";
        case TX_VALID_ERR_AMOUNT_INVALID:    return "amount invalid";
        case TX_VALID_ERR_AMOUNT_NEGATIVE:   return "amount negative";
        case TX_VALID_ERR_AMOUNT_OVERFLOW:   return "amount overflow";
        case TX_VALID_ERR_CURRENCY_UNSUPPORTED: return "currency unsupported";
        case TX_VALID_ERR_NULL_PTR:          return "null ptr";
        default:                             return "unknown";
    }
}
