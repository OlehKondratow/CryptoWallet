\page task_display_minimal "task_display_minimal: minimal-lwip display + log mirroring"
\related Task_Display_Create
\related Task_Display_Log

# `task_display_minimal.c`

<brief>The `task_display_minimal` module is a lightweight UI/logging implementation for `minimal-lwip`: it minimizes SSD1306 load, mirrors messages to UART, and writes a short log tail to `g_display_ctx`, so the display can be updated only when necessary.</brief>

## Overview

The `task_display_minimal` module is a lightweight UI/logging implementation for `minimal-lwip`: it minimizes SSD1306 load, mirrors messages to UART, and writes a short log tail to `g_display_ctx`, so the display can be updated only when necessary.

## Logic Flow (Minimal OLED Loop)

Task flow:
1. At startup, logs start message and (if `!SKIP_OLED`) performs quick output "+ LwIP / DHCP..."
2. Then task enters infinite loop:
   - blinks LED1 as "alive" indicator
   - optionally prints "Disp: alive" every 5 seconds (if `LWIP_ALIVE_LOG`)
   - if OLED enabled (`!SKIP_OLED`), periodically reads `g_display_ctx` under mutex and updates screen (IP + log tail)
   - delay `500ms` between updates

### Flag Branching Scenarios

| Flag | Effect on Display |
|------|------------------|
| `SKIP_OLED=1` | I2C/SSD1306 traffic completely disabled; task remains with only alive/logging |
| `LWIP_ALIVE_LOG` | Adds periodic UART+log alive to see network liveliness |

## Interrupts and Registers

No direct registers/ISR. Only "low-level" part: direct SSD1306 driver calls over I2C, protected by mutex.

## Timings and Branching Conditions

| Operation | Value |
|-----------|-------|
| creation/startup | `vTaskDelay(100ms)` after display start |
| I2C/ctx mutex | `xSemaphoreTake(..., 50ms)` / `xSemaphoreTake(..., 20ms)` |
| OLED update loop | `vTaskDelay(500ms)` |
| UI queue | not used in minimal build (UI merge functions stubbed) |

## Dependencies

- Global UI context structure: `g_display_ctx` and `g_display_ctx_mutex` (for log line + IP string)
- Shared I2C resource: `g_i2c_mutex`
- Logging: `UART_Log()` (via `Task_Display_Log()`)
- SSD1306: `ssd1306_*` + `Font_6x8`
- LED policy (indirect): blinks LED1 as display liveliness indicator

## Module Relationships

- `task_display.md` (full UI version)
- `task_net.md` / `app_ethernet_cw.md` (provide IP/DHCP that gets displayed)
- `hw_init.md` (I2C/SSD1306 basic init in main path)
