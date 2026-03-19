\page task_security "task_security: legacy/mock signing FSM"
\related Task_Security_Create

# `task_security.c` + `task_security.h`

<brief>The `task_security` module holds a "legacy/mock" variant of the signing FSM for bring-up and comparison: it is not used in the main path (`main.c` does not call it), but implements similar confirmation automation and replaces cryptography with stubs.</brief>

## Overview

The `task_security` module holds a "legacy/mock" variant of the signing FSM for bring-up and comparison: it is not used in the main path (`main.c` does not call it), but implements similar confirmation automation and replaces cryptography with stubs.

## Logic Flow (Legacy FSM)

Internal state machine is implemented as:
1. Local variable `state` starts in `SIGNING_IDLE`
2. In IDLE mode, the task takes payload from `g_tx_queue` (timeout 200ms) and transitions to `SIGNING_RECEIVED`
3. Then on each tick, one iteration of `fsm_signing_step` is executed, which processes:
   - RECEIVED: performs mock-hash and transitions to WAIT_CONFIRM
   - WAIT_CONFIRM: waits for confirm/reject event bits with 30s timeout
   - IN_PROGRESS: performs mock-sign and transitions to DONE (or ERROR)
4. When reaching terminal states (DONE/REJECTED/ERROR), the task clears tx and returns to IDLE

### Display State Update

The module includes a helper "update signing on display context" which maps FSM state to `g_display_ctx` fields (locked/valid/pending).

## Interrupts and Registers

No ISR/register access. Module uses:
- event group wait
- queue payload
- memory clear `memzero()` for sensitive buffers on exit paths

## Timings and Branches

| Parameter | Value |
|-----------|-------|
| timeout on queue receive | 200ms |
| confirm/reject timeout | 30000ms |
| processing | per loop iterations (no strict periodic sleep, only delays from queues/waits) |

Key branching:
- reject/timeout → SIGNING_REJECTED (or return via terminal state)
- no reject → CONFIRM → SIGNING_IN_PROGRESS → SIGNING_DONE

## Dependencies

Direct dependencies:
- `g_tx_queue` / `wallet_tx_t` (input)
- `g_user_event_group` / `EVENT_USER_CONFIRMED/REJECTED` (UX signal)
- `g_display_ctx_mutex` and `g_display_ctx` (status output)
- `memzero()` (sanitization)
- Mock-crypto functions inside module (not real crypto peripherals)

Architectural peculiarity:
- In `main.c`, `Task_Security_Create()` is not called; so the module by default does not affect production path

## Module Relationships

- `task_sign.md` (production FSM, real cryptography)
- `task_user.md` (source event bits)
- `wallet_shared.md` (common FSM state contract + event bits)
- `task_display.md` (context/logging display layer)
