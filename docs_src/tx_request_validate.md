\page tx_request_validate "tx_request_validate: validation gate for signing"
\related tx_request_validate
\related tx_recipient_format_ok
\related tx_amount_format_ok
\related tx_currency_supported

# `tx_request_validate.c` + `tx_request_validate.h`

<brief>The `tx_request_validate` module is a guard gate before signing: it checks host-supplied fields (recipient address, amount, currency) for basic format compliance, helping avoid obviously wrong data on SSD1306/in logs and protecting the signing pipeline from garbage input.</brief>

## Overview

The `tx_request_validate` module is a guard gate before signing: it checks host-supplied fields (recipient address, amount, currency) for basic format compliance, helping avoid obviously wrong data on SSD1306/in logs and protecting the signing pipeline from garbage input.

## Logic Flow (Validation Gates)

Function `tx_request_validate()` performs sequential checks:
1. Null-check on object itself
2. **Recipient:** non-empty, length < `TX_RECIPIENT_LEN`, format OK (Base58 for P2PKH/P2SH or bech32 for SegWit)
3. **Amount:** non-empty, is decimal string, contains at least one digit, max one decimal point, `atof` > 0
4. **Currency:** if non-empty, check against whitelist (BTC, ETH, LTC, BCH, DOGE, DASH, XMR); if empty, default to BTC
5. On first error, return corresponding enum `TX_VALID_ERR_*`

Each check has its helper function (`tx_recipient_format_ok`, `tx_amount_format_ok`, `tx_currency_supported`).

## Interrupts and Registers

No ISR/registers: module is pure string validation logic.

## Timings and Branching Conditions

| Check | Failure Condition | Error Code |
|-------|-------------------|-----------|
| recipient empty | `recipient[0] == '\0'` | `TX_VALID_ERR_RECIPIENT_EMPTY` |
| recipient too long | `len >= TX_RECIPIENT_LEN` | `TX_VALID_ERR_RECIPIENT_TOO_LONG` |
| recipient bad format | not Base58 (for '1'/'3') or not bech32 (for "bc1") | `TX_VALID_ERR_RECIPIENT_INVALID_CHARS` |
| amount empty | `amount[0] == '\0'` | `TX_VALID_ERR_AMOUNT_EMPTY` |
| amount not decimal | contains non-digit/non-dot chars | `TX_VALID_ERR_AMOUNT_INVALID_CHARS` |
| amount zero/negative | `atof(amount) <= 0` | `TX_VALID_ERR_AMOUNT_ZERO` |
| currency unsupported | not in whitelist and not empty | `TX_VALID_ERR_CURRENCY_UNSUPPORTED` |

## Dependencies

Direct:
- Input: `wallet_tx_t` with recipient/amount/currency fields
- Output: `tx_validate_result_str()` for human-readable error messages
- No external crypto or hardware dependencies

Indirect:
- `task_net.md` (calls validation on received HTTP request)
- `task_sign.md` (receives validated tx from queue)

## Module Relationships

- `task_net.md` (requests validation before enqueue)
- `task_sign.md` (consumes validated transactions)
- `wallet_shared.md` (transaction data contract)
