\page task_sign "task_sign: transaction signing pipeline + confirm FSM"
\related Task_Sign_Create
\related g_tx_queue
\related g_user_event_group
\related g_last_sig_ready

# `task_sign.c` + `task_sign.h`

<brief>The `task_sign` module implements the core signing pipeline: it receives a validated request from a queue, forms a deterministic input for SHA-256, waits for user confirmation/rejection, and upon success, stores a compact signature and prepares it for network/USB response.</brief>

## Overview

The `task_sign` module is the central point of "policy and cryptography" in the project: it connects network data with human confirmation and the cryptographic signing operation. Its business role is to guarantee that cryptographic operations do not start before input string validity and do not complete without an explicit `Confirm` event from `task_user`. On errors, it transitions the system to a safe UX mode (via `g_security_alert` and display context update).

## Logic Flow (Explicit Signing FSM)

Internal logic is organized through the `signing_state_t` enum and a local `state` variable. Semantic states and transitions:

### 1) SIGNING_IDLE

Steps:
1. Wait for transaction from `g_tx_queue` (timeout 200ms).
2. Validate host payload via `tx_request_validate`.
3. On validation error: log, set `g_security_alert=1`, clear temporary buffers, and return to `IDLE`.
4. On success: transition to `SIGNING_RECEIVED`.

### 2) SIGNING_RECEIVED

Steps:
1. Update display context (currency/amount, safe locked=false while waiting for confirmation).
2. Form deterministic SHA-256 input from recipient/amount/currency fields.
3. Calculate SHA-256 (error → security_alert=1 and return to IDLE).
4. Transition to `SIGNING_WAIT_CONFIRM`.

### 3) SIGNING_WAIT_CONFIRM

Steps:
1. Clear event group bits `EVENT_USER_CONFIRMED | EVENT_USER_REJECTED`.
2. Wait for one of the events with timeout `CONFIRM_TIMEOUT_MS=30000ms`.
3. Branches:
   - `REJECTED` → log "Rejected", clear, return to IDLE
   - no `CONFIRMED` (timeout) → log "Timeout", clear, return to IDLE
4. On confirm:
   - get seed (`get_wallet_seed`) and derive key m/44'/0'/0'/0/0
   - cryptographic signature `crypto_sign_btc_hash(...)`
   - store `g_last_sig` and set `g_last_sig_ready=1`
   - zero sensitive buffers
   - reset `g_security_alert=0`
   - update display context to locked/valid and clear pending flag via `UI_ClearPending()`

## Interrupts and Registers

No direct ISR/register-level operations. However, there is a critical security aspect:
- Sensitive buffers are cleared via `memzero()` on all exit paths (including error branches)
- Key "safe behavior" checkpoints are tied to event groups and queues

## Timings and Branching Conditions

| Moment | Value | What It Controls |
|--------|-------|------------------|
| queue payload wait | 200ms | avoid blocking forever on empty queue |
| display context mutex wait | 50ms | avoid hangs on UI mutex problems |
| user confirmation | 30000ms | timeout denies the signature |
| queue/merge display pending | via `UI_ClearPending()` | transition UX from confirm mode |

## Dependencies

Direct dependencies:
- Input data: `g_tx_queue` (`wallet_tx_t`)
- User signal: `g_user_event_group` + bits `EVENT_USER_CONFIRMED/REJECTED`
- Validation: `tx_request_validate()` and `tx_validate_result_str()`
- Crypto layer: `crypto_hash_sha256()`, `crypto_derive_btc_m44_0_0_0_0()`, `crypto_sign_btc_hash()`
- Seed: `get_wallet_seed()` (weak stub in this module; real implementation may be in `wallet_seed.c` for test builds)
- UI/logging: `Task_Display_Log()`, `UI_ClearPending()` and protected access to `g_display_ctx`

Global structures/flags that are modified:
- `g_last_sig`, `g_last_sig_ready`
- `g_security_alert`
- fields of `g_display_ctx` (currency/amount/safe_locked/signature_valid)

## Module Relationships

- `task_net.md` (creates/puts payload in `g_tx_queue`)
- `tx_request_validate.md` (validation)
- `task_user.md` (confirm/reject event bits)
- `task_display.md` (pending/UX rendering)
- `crypto_wallet.md` / `memzero.md` (crypto and sanitization)
