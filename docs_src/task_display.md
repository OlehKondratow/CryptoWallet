\page task_display "task_display: SSD1306 UI (4-line scroll) + UI merge"
\related Task_Display_Create
\related UI_UpdateData
\related Task_Display_Log
\related UI_ClearPending

# `task_display.c` + `task_display.h`

<brief>The `task_display` module manages the visual state of the wallet on SSD1306: it receives network/signing events, merges them into a single displayable state, and renders 4 lines of UI with "scrolling text" for logs and network data.</brief>

## Overview

The `task_display` module manages the visual state of the wallet on SSD1306: it receives network/signing events, merges them into a single displayable state, and renders 4 lines of UI with "scrolling text" for logs and network data.

## Logic Flow (UI Rendering + Queue/Event Merge)

Main task loop:
1. Initialize internal buffers
2. Wait: task attempts to extract pending transaction "snapshot" from queue with time limit
3. Update internal UI context (pending flag + transaction data) with mutex protection
4. Render current 4 lines on SSD1306
5. Periodic delay for "tick rate" of update/scroll

### UI States (How It "Lives")

No explicit enum state machine like "classic" automation, but conditional layout on 4 lines:

| Line | What It Displays | Movement/Scroll |
|------|-----------------|-----------------|
| 0 | WALLET: coin, amount, shortened recipient + mode indicator `Confirm?` | static (no scroll) |
| 1 | SECURITY: Safe locked/unlocked + signature status/hint | static |
| 2 | NETWORK: IP + MAC | scroll (if line longer) |
| 3 | LOG: tail of recent messages | scroll |

Scroll offset implemented via `s_line_offset[row]`:
- lines 0-1: offset forced to 0
- lines 2-3: offset incremented modulo line length

## Interrupts and Registers

Module is FreeRTOS task, no direct register work. Only "register-like" part: accessing SSD1306 UI driver, which hides low-level work via I2C.

Critical locks:

| Resource | How Protected | Where |
|----------|---------------|-------|
| I2C | take `g_i2c_mutex` (in render_lock/unlock) | filling/outputting to SSD1306 |
| Common UI data | `g_ui_mutex` | merging pending/contexts |
| display context | `g_display_ctx_mutex` | updating log tail |

## Timings and Branches

| Parameter | Value | Meaning |
|-----------|-------|---------|
| display queue | timeout `QUEUE_WAIT_MS=100ms` | pending updates don't block rendering long |
| log scroll | `LOG_SCROLL_MS=120ms` | "tick rate" of display speed |
| I2C lock | `portMAX_DELAY` | ensures atomic output to OLED |
| UI mutex | wait `pdMS_TO_TICKS(50)` / render `pdMS_TO_TICKS(20)` | prevents hangs, limits max delays |

Branches:
- if pending queue didn't give data — UI stays in current state and just keeps rendering lines
- `Confirm?` dynamics depend on `pending_tx.is_pending`

## Dependencies

Direct dependencies:
- Global channels: `g_display_queue`, `g_ui_mutex`, `g_i2c_mutex`, `g_display_ctx_mutex`, internal structures `s_ui_data`
- Common UI types: `Transaction_Data_t`, `UI_Display_Data_t` (from `task_display.h`)
- Display driver: `ssd1306_*` and `Font_6x8`

Indirectly affects/coordinates with:
- network/HTTP: via placing `pending_tx` in queue (in `task_net.c` / request logic)
- signing: when after confirmation/rejection pending transitions from confirm-mode (via `UI_ClearPending()`)

## Module Relationships

- `task_display_minimal.md` (lightweight version for minimal-lwip)
- `task_net.md` (pending snapshot generation)
- `task_sign.md` (clear pending and change security/lock UX)
- `task_user.md` (confirmation/rejection via event group)
- `hw_init.md` (I2C/SSD1306 base init)
