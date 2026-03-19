\page ssd1306_conf "ssd1306_conf: SSD1306 driver build-time configuration"

# `Drivers/ssd1306/ssd1306_conf.h`

<brief>Header `ssd1306_conf` defines build-time configuration for the SSD1306 driver: binds I2C1 (`hi2c1`) and address 0x3C, sets display geometry 128×32, and enables the required font (Font 6×8). These macros are consumed by driver sources and calling UI code.</brief>

## Overview

<brief>Header `ssd1306_conf` defines build-time configuration for the SSD1306 driver: binds I2C1 (`hi2c1`) and address 0x3C, sets display geometry 128×32, and enables the required font (Font 6×8). These macros are consumed by driver sources and calling UI code.</brief>

## Abstract (Logic Synthesis)

An SSD1306 driver on bare-metal is typically compiled "for a specific display layout" and "for a specific bus/address". `ssd1306_conf.h` is the fixation point for such parameters, so that `task_display` and `task_display_minimal` can work with a single `ssd1306_*` interface without dealing with compilation settings.

## Logic Flow

This is not a runtime module, but a set of macros:

| Macro | Value | Role |
|---|---|---|
| `SSD1306_USE_I2C` | defined | selects I2C backend |
| `SSD1306_I2C_PORT` | `hi2c1` | binding to HAL I2C instance |
| `SSD1306_I2C_ADDR` | `(0x3C<<1)` | 7-bit address with I2C shift factor |
| `SSD1306_WIDTH`/`HEIGHT` | `128/32` | dimensions for buffer/rendering |
| `SSD1306_INCLUDE_FONT_6x8` | defined | presence of required font |

## Interrupts/Registers

No ISRs or registers: only configuration macros for enabling correct driver code.

## Dependencies

Direct:
- `hi2c1` from `hw_init.c` (exported in `hw_init.h`)

Applied to:
- `task_display.c`, `task_display_minimal.c` (via `ssd1306.h` and font includes)

## Relations

- `hw_init.md` (I2C init + `hi2c1`)
- `task_display.md` (rendering over SSD1306)
