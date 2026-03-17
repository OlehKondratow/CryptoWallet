# Secure Crypto Wallet

Modular FreeRTOS-based firmware for a Secure Crypto Wallet on **STM32H743ZI2 (Nucleo-144)**.

## Architecture

- **Core**: `main.c` — Hardware init and FreeRTOS scheduler start only.
- **Display** (`task_display`): SSD1306 (I2C1, PB8/PB9, 128×32, 0x3C) state machine.
- **Security** (`task_security`): Transaction decode, User Key confirm, mock SHA256/signing.
- **Network** (`task_net`): LwIP + HTTP server (optional, `USE_LWIP`).
- **IO** (`task_io`): LEDs (Green/Yellow/Red), User Button (PC13) with debouncing.

## Dependencies

- **STM32CubeH7** (sibling `../STM32CubeH7`)
- **stm32-ssd1306** (sibling `../stm32-ssd1306`)
- **stm32_secure_boot** (sibling `../stm32_secure_boot`) — linker script, FreeRTOS

## Build

```bash
make
```

## Documentation

MkDocs + Material theme. Source: `docs_src/*.md`

```bash
# One-time: create venv and install
python3 -m venv .venv-docs
.venv-docs/bin/pip install -r requirements-docs.txt

make docs          # → docs/index.html
make docs-serve    # live reload at http://127.0.0.1:8000
```

## Flash

```bash
make flash
```

## Build options

**DHCP timeout**: If DHCP fails, static fallback 192.168.0.10 is used. Adjust `IP_ADDR0..3`, `NETMASK_*`, `GW_*` in `Core/Inc/main.h` to match your network. For static-only: set `LWIP_DHCP 0` in `Core/Inc/lwipopts.h`.

**Verbose log**: `make LWIP_ALIVE_LOG=1` enables UART heartbeat ("Disp: alive", "Net: alive").

**Skip OLED**: `make SKIP_OLED=1` if I2C hangs.

**Boot test** (diagnostic: no FreeRTOS, LED blink only):
```bash
make boottest
make flash-boottest
```
Expected: UART "CryptoWallet + LwIP" and "boot" every 500ms, LED1 blink.

**GDB/st-util** (debug + flash): Run from project root:
```bash
# Terminal 1: st-util
st-util

# Terminal 2: build + GDB (load flashes the device)
make
arm-none-eabi-gdb -x gdb_minimal_lwip.gdb

# Or manually:
arm-none-eabi-gdb build/cryptowallet.elf
(gdb) target extended-remote :4242
(gdb) monitor reset halt
(gdb) load
(gdb) c
# When UART shows "Net: start" and hangs: Ctrl+C in GDB
(gdb) bt          # backtrace — where it's stuck
(gdb) info threads
```

## Display States

| State      | Content                                      |
|-----------|-----------------------------------------------|
| WALLET    | Currency type and amount                      |
| SECURITY  | Safe status (Locked/Unlocked), signature      |
| NETWORK   | IP/MAC, HID connection                        |
| LOG       | Scrollable system log                         |

## Inter-Task Communication

- **Queue**: `task_net` → `task_security` (transaction data)
- **Event Group**: `task_io` → `task_security` (User Confirmed)
- **Mutex**: I2C access for display

## Pinout (Nucleo-H743ZI2)

| Function   | Pin  |
|-----------|------|
| LED1 (Green)  | PB0  |
| LED2 (Yellow) | PE1  |
| LED3 (Red)    | PE2  |
| User Key      | PC13 |
| I2C1 SCL      | PB8  |
| I2C1 SDA      | PB9  |
