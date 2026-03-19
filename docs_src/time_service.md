\page time_service "time_service: SNTP -> epoch + UTC formatting"
\related time_service_init
\related time_service_start
\related time_service_now_epoch

# `time_service.c` + `time_service.h`

<brief>The `time_service` module provides time synchronization via SNTP and gives the application unified access to current Unix epoch and UTF string representation (for logs/UI), built on top of `HAL_GetTick()` after epoch received from network.</brief>

## Overview

The `time_service` module provides time synchronization via SNTP and gives the application unified access to current Unix epoch and UTC string representation for logs/UI, built on top of `HAL_GetTick()` after epoch received from network.

## Logic Flow (SNTP Lifecycle)

Lifecycle:
1. Initialization: reset flags and base values; fix current `HAL_GetTick()` as starting anchor
2. Start SNTP client: module runs only once (guard `s_sntp_started`), initialization via `tcpip_callback` so code lives in LwIP context
3. Get epoch: on synchronization, callback stores `s_epoch_base` and `s_tick_base_ms`, sets `s_time_synced`
4. Anytime:
   - compute "now epoch" as `epoch_base + (HAL_GetTick()-tick_base)/1000`
   - format UTC string (if `s_time_synced`, else print `UNSYNCED`)

### Epoch → UTC Algorithm

Conversion without RTC: divide by days/seconds and sequential subtraction by years/months, accounting for leap years.

| Step | How Calculated |
|------|----------------|
| Decomposition | epoch_sec → days + remainder → hour/minute/second |
| Years | subtract `365/366` while days still in current year |
| Months | subtract days by month table, correcting February for leap year |

## Interrupts and Registers

No direct registers. But there is asynchrony:
- SNTP start initiated via `tcpip_callback` (callback expected to run in LwIP TCP/IP thread context)
- "now epoch" calculation tied to `HAL_GetTick()`, i.e., system tick/HAL timer

## Timings and Branching Conditions

| Guard/Flag | Condition | Behavior |
|------------|-----------|----------|
| `s_sntp_started` | repeated call to start | start ignored |
| `s_time_synced` | request string/time | `time_service_now_string` returns `UNSYNCED`, epoch calculation bases on last epoch_base |

Critical point:
- `time_service_start()` returns "success" only on fact of posting callback to `tcpip_callback`; network result comes later

## Dependencies

Direct:
- LwIP: `lwip/apps/sntp.h`, `lwip/tcpip.h` (SNTP init + run in TCP/IP thread)
- Time/timebase: `HAL_GetTick()`
- UI/logging: `Task_Display_Log()` on start/errors/sync

Global structures/flags:
- local static flags/bases: `s_sntp_started`, `s_time_synced`, `s_epoch_base`, `s_tick_base_ms`

## Module Relationships

- `main.md`: starts service init and initiates bootstrap
- `task_net.md` / `app_ethernet_cw.md`: usually trigger "after link" moment (when to call `time_service_start()`)
- `task_display.md`: shows time strings/log tails (if passed to it)
