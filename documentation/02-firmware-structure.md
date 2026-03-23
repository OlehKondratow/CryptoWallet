# 2. Firmware structure

## 2.1 MCU and layout

- **MCU:** STM32H743ZI (Cortex-M7), NUCLEO-H743ZI2 target.
- **Flash:** Application linked for LwIP build in `STM32H743ZITx_FLASH_LWIP.ld` (see Makefile `LDSCRIPT`).
- **OS:** FreeRTOS (CMSIS-RTOS2 API in places).

## 2.2 Initialization

- **`Core/Src/main.c`** — clock/RTOS start, creates tasks and IPC primitives.
- **`Core/Src/hw_init.c`** — clocks, MPU/cache rules needed for Ethernet DMA, GPIO, I2C (OLED), UART, USB, RNG peripheral as required by build flags.

## 2.3 Tasks (conceptual)

| Area | Typical files | Role |
|------|----------------|------|
| Display | `task_display.c`, `task_display_minimal.c` | OLED UI, transaction summary |
| Network | `Src/task_net.c` | HTTP server, request parsing, queues to sign |
| Signing | `task_sign.c` | Validates request, shows UI, ECDSA, response |
| User / IO | `task_user.c`, `task_io.c` | Button debounce, LEDs |
| USB | `usb_device.c`, `usb_webusb.c`, `usbd_*_cw.c` | WebUSB when `USE_WEBUSB=1` |
| UART | `cwup_uart.c`, `rng_dump.c` | CWUP vs raw RNG stream (mutually exclusive on same UART) |
| Integrity | `fw_integrity.c` | Startup + optional `AT+FWINFO?` |

Legacy or audit paths may include `task_security.c`—see source before relying on it in product flows.

## 2.4 IPC and shared types

- **`Core/Inc/wallet_shared.h`** — transaction types, queue handles, display context, limits (`TX_*_LEN`).
- **Queues** — e.g. network → sign → display; sizes and drop policy are part of correctness under load (see source).

## 2.5 Module map (source of truth)

| Concern | Primary files |
|---------|----------------|
| HTTP routes | `Src/task_net.c` |
| Request validation | `Core/Src/tx_request_validate.c` |
| Crypto wrapper | `Core/Src/crypto_wallet.c` |
| Secure zero | `Core/Src/memzero.c` |
| Ethernet / DHCP | `Src/app_ethernet_cw.c` |
| Time | `Core/Src/time_service.c` |
| Faults | `Core/Src/fault_report.c`, `stm32h7xx_it*.c` |

## 2.6 Third-party

- **`ThirdParty/trezor-crypto`** — BIP-39/32, secp256k1, hashes (when `USE_CRYPTO_SIGN=1`).
- **STM32 HAL / CMSIS** — via `STM32CubeH7` tree (`CRYPTO_DEPS_ROOT` or sibling checkout).
