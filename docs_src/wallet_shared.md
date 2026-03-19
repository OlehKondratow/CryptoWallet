\page wallet_shared "wallet_shared: IPC/data contracts for net/display/sign/user"
\related g_tx_queue
\related g_display_queue
\related g_user_event_group
\related g_i2c_mutex
\related g_ui_mutex
\related g_display_ctx_mutex

# `wallet_shared.h` (data contract + IPC)

<brief>The `wallet_shared` header defines a unified contract between modules: data structures for "request → confirmation → signature" and UI state, as well as global IPC handles (queues, event groups, mutexes) that are exchanged between `task_net`, `task_sign`, `task_user`, and `task_display`.</brief>

## Overview

The `wallet_shared` header defines a unified contract between modules: data structures for "request → confirmation → signature" and UI state, as well as global IPC handles (queues, event groups, mutexes) that are exchanged between `task_net`, `task_sign`, `task_user`, and `task_display`.

## Logic Flow

In the architecture of an embedded system, "business logic" is distributed across tasks, and `wallet_shared.h` is the layer of contracts: how exactly the network module turns into an object understood by the signing module, how a UX button turns into a discrete event, and how the secure/signed wallet status turns into what SSD1306 draws. Without this contract, any distribution of tasks across threads turns into chaotic data passing.

## Logical Model

No explicit state machine in the header, but there is a "formal" model:
1. Network/host forms `wallet_tx_t` (recipient/amount/currency)
2. `task_sign` consumes `wallet_tx_t` from `g_tx_queue` and transitions UI via `g_display_ctx`
3. `task_user` sets events in `g_user_event_group`, which `task_sign` waits for
4. UI gets updates either via `g_display_queue` or via `g_display_ctx`

## Interrupts and Registers

The header only declares types/global descriptors. Contains no register/ISR.

## Dependencies

Key data dependencies:
- Types and string sizes agreed between validation/render:
  - `TX_RECIPIENT_LEN`, `TX_AMOUNT_LEN`, `TX_CURRENCY_LEN`
- User events:
  - `EVENT_USER_CONFIRMED`, `EVENT_USER_REJECTED` (passed to `task_sign`)
- UI states/context:
  - `display_state_t` (set of screens) and `display_context_t` (data for rendering)
- Signing FSM states:
  - `signing_state_t` as common enum (both for active task_sign and legacy task_security)

Global handles (created in `main.c`, used in tasks):

| Handle | Type | Usually Writes | Reads/Waits |
|--------|------|-----------------|------------|
| `g_tx_queue` | `QueueHandle_t` | `task_net` | `task_sign` |
| `g_display_queue` | `QueueHandle_t` | `task_net` | `task_display` |
| `g_user_event_group` | `EventGroupHandle_t` | `task_user` | `task_sign` |
| `g_i2c_mutex` | `SemaphoreHandle_t` | display | display-only critical section |
| `g_ui_mutex` | `SemaphoreHandle_t` | task_display | merge context |
| `g_display_ctx_mutex` | `SemaphoreHandle_t` | net/sign | read in display |

Global state/result variables:
- `g_security_alert` — indicator mapped to LED3
- `g_last_sig[64]` and `g_last_sig_ready` — last signature, polled by network/USB layer

## Module Relationships

- Signature consumer: `task_sign.md`
- UX events consumer: `task_user.md`
- Host payload validation: `tx_request_validate.md`
- UI display: `task_display.md`
- Alert display policy: `task_io.md`
- Network payload assembly/queuing: `task_net.md`, `app_ethernet_cw.md`
