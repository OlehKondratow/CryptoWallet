# Build & Flash

## Dependencies

- **STM32CubeH7** (sibling `../STM32CubeH7`)
- **stm32-ssd1306** (sibling `../stm32-ssd1306`)
- **stm32_secure_boot** (sibling `../stm32_secure_boot`) — linker script, FreeRTOS

## Build

```bash
make
```

## Build options

- **SKIP_OLED=1** — skip OLED if I2C hangs
- **LWIP_ALIVE_LOG=1** — UART heartbeat
- **Boot test**: `make boottest` — no FreeRTOS, LED blink only

## GDB/st-util (debug + flash)

```bash
# Terminal 1: st-util
st-util

# Terminal 2: build + GDB (load flashes the device)
make
arm-none-eabi-gdb -x gdb_minimal_lwip.gdb
```
