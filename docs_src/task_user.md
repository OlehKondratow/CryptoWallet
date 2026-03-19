\page task_user "task_user: USER button policy (confirm/reject)"
\related Task_User_Create

# `task_user.c` + `task_user.h`

<brief>The `task_user` module implements physical UX logic for the USER button (PC13): performs debounce, distinguishes short press (Confirm) from long hold (~2.5s) as Reject, and signals this to `task_sign` via `g_user_event_group`.</brief>

## Overview

The `task_user` module implements physical UX logic for the USER button (PC13): performs debounce, distinguishes short press (Confirm) from long hold (~2.5s) as Reject, and signals this to `task_sign` via `g_user_event_group`.

## Logic Flow (Button State Machine)

Approach: periodic polling with stability filtering.

Triggers and thresholds:

| Parameter | Value |
|-----------|-------|
| `POLL_MS` | 20ms |
| Debounce | 50ms |
| Long press | 2500ms |

States (implicit, via variables):
1. Idle (button not pressed)
2. Pressed-Stabilizing (button pressed, accumulating stable ticks)
3. Confirm-Fired (short press confirmed on release)
4. Reject-Fired (long hold rejects immediately during hold)

Rules:
1. While button remains pressed — accumulates `btn_stable_ticks`
2. If hold reaches `LONG_PRESS_MS` and action not yet "fired" — sets `EVENT_USER_REJECTED`
3. On release:
   - if duration in range `[DEBOUNCE_MS, LONG_PRESS_MS)` and action not yet "fired" — sets `EVENT_USER_CONFIRMED`
   - then resets duration flags

## Interrupts and Registers

No direct ISR: module works as FreeRTOS task and reads GPIO state via HAL.
Timing precision at level of `POLL_MS` (20ms): event may shift no more than one tick.

## Timings and Branching Conditions

| Branch | Condition | Result |
|--------|-----------|--------|
| Reject | `last_press_duration >= 2500 && !action_fired` | set `EVENT_USER_REJECTED` |
| Confirm | `DEBOUNCE_MS <= last_press_duration < 2500 && !action_fired` on release | set `EVENT_USER_CONFIRMED` |
| Debounce | while duration not reached 50ms | no events set |

## Dependencies

Direct:
- Input: `USER_KEY_GPIO_PORT`, `USER_KEY_PIN`, `USER_KEY_PRESSED` (from `main.h`)
- Events: `g_user_event_group` + bits `EVENT_USER_CONFIRMED`, `EVENT_USER_REJECTED` (from `wallet_shared.h` / task architecture)
- Logging: `Task_Display_Log`

Indirect:
- `task_sign` waits for these event bits and transitions system to confirm/reject scenario

## Module Relationships

- `task_sign.md` (consumer of `g_user_event_group`)
- `task_io.md` (security alert indication after decision)
- `task_display.md` (Confirm/Reject labels via logging layer)
