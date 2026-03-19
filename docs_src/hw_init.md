\page hw_init "hw_init: board bring-up (clock/MPU/GPIO/I2C/UART/USB)"
\related HW_Init
\related HW_Init_Early_LwIP
\related UART_Log

# `hw_init.c` + `hw_init.h`

<brief>The `hw_init` module handles low-level board bring-up: it establishes proper clock/cache/MPU ordering (critical for LwIP/ETH), initializes GPIO for UX (LED/button), sets up I2C1 for SSD1306 and UART logging, plus optional USB and RNG.</brief>

## Overview
<brief>The `hw_init` module handles low-level board bring-up: it establishes proper clock/cache/MPU ordering (critical for LwIP/ETH), initializes GPIO for UX (LED/button), sets up I2C1 for SSD1306 and UART logging, plus optional USB and RNG.</brief>

## Abstract (Logic Synthesis)
`hw_init` is a "trust layer" between HAL/Cube and the application: it ensures that before any network logic (when `USE_LWIP` is enabled), MPU and cache are correctly configured for DMA and memory regions; it then guarantees that peripherals needed by tasks (I2C/OLED, UART, and basic GPIO) are available in their expected state.

## Logic Flow (bootstrap sequence)
Two critical entry points exist:
1. `HW_Init_Early_LwIP()` runs before `HAL_Init()` and is active only when `USE_LWIP` is set.
2. `HW_Init()` runs after `HAL_Init()` and handles the main bring-up: clocks, GPIO, I2C1, UART3, and optional USB/RNG; OLED initialization depends on `SKIP_OLED`.

## Detailed Analysis

### Purpose and Responsibility Boundaries
`hw_init` wraps HAL initialization. By design:
- `main.c` prepares IPC/tasks and launches FreeRTOS, calling init functions in correct sequence.
- `hw_init.c` maintains "hardware" order: clocks, cache/MPU, peripherals (GPIO/I2C/UART), and build-flag dependencies.
- Network logic (Ethernet PHY/DHCP/link) doesn't live here directly: it's enabled by the LwIP/Cube environment and implemented in other modules (e.g., via Cube/BSP + `Src/app_ethernet_cw.c`).

### Entry Point #1: `HW_Init_Early_LwIP()` (only when `USE_LWIP`)
This function is called from `main.c` **before** `HAL_Init()` to ensure Cortex-M7 correctly handles DMA/memory regions used by Ethernet/LwIP.

Key logic inside:
1. `HAL_MPU_Disable()` — safely reconfigure regions.
2. Configure three MPU regions:
   - **Region 0**: "deny all" (4GB no-access) as default.
   - **Region 1**: address **`0x30000000`** (1 KB) for **ETH descriptors** (DMA descriptors).
   - **Region 2**: address **`0x30004000`** (16 KB) for **LwIP heap**.
3. `HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT)` — enable MPU.
4. Enable caches: `SCB_EnableICache()` and `SCB_EnableDCache()`.

Why this matters for DMA:
- Ethernet DMA reads/writes memory; if descriptor/heap addresses aren't properly configured, cache can intercept data, causing desynchronization.
- Specific base addresses/sizes are tuned to the LwIP/Cube pipeline (template `lwip_zero`).

### Entry Point #2: `HW_Init()` (after `HAL_Init()`)
`HW_Init()` — main bring-up after `HAL_Init()`:

1. **Build without LwIP (`!USE_LWIP`)**
   - Calls `MPU_Config()` (currently `HAL_MPU_Disable()`; avoids keeping MPU active unnecessarily).
   - `CPU_CACHE_Enable()` runs later, after clock switching (recommended for H7).

2. **Post-enable delay** (empty loop ~2M iterations)
   - Comment indicates MCO/ST-Link clocking stabilization on Nucleo.

3. **`SystemClock_Config()`**
   - Enables power (PWR regulator scale1).
   - Enables **D2 SRAM3** clock: `__HAL_RCC_D2SRAM3_CLK_ENABLE()`.
   - Configures PLL from HSE, sets AHB/APB dividers. Ensures correct frequencies for HAL and peripherals.

4. `SystemCoreClockUpdate()`
   - Synchronizes HAL variables with actual clocking after SYSCLK switch.

5. Peripheral initialization:
   - `MX_GPIO_Init()`:
     - Enables clocks for LED1/LED2/LED3 ports and `USER` button.
     - Configures LEDs as `GPIO_MODE_OUTPUT_PP`, low speed, "off" levels.
     - Configures button as `GPIO_MODE_INPUT`.
   - `MX_I2C1_Init()`:
     - Configures `hi2c1` (Timing matching reference, 7-bit addressing).
     - Enables analog filter, disables digital.
     - Calls `ssd1306_Init()` immediately after `MX_I2C1_Init()`, only when `USE_LWIP && !SKIP_OLED`.
   - `MX_USART3_Init()`:
     - `USARTx` at 115200, 8N1, TX/RX mode.
     - No hardware flow control, oversampling 16.

6. Optional branches:
   - `USE_WEBUSB`:
     - Log via UART through `Task_Display_Log()`.
     - `MX_USB_Device_Init()`.
     - Second log "USB ready".
   - `USE_CRYPTO_SIGN`:
     - `MX_RNG_Init()` — hardware RNG via HAL (`hrng` + `HAL_RNG_Init()`).
     - Also provides `HAL_RNG_MspInit()` for clock enable `__HAL_RCC_RNG_CLK_ENABLE()`.

### Interaction with `main.c` and Other Modules
In `Core/Src/main.c`, order is:
1. `HW_Init_Early_LwIP()` (only when `USE_LWIP`)
2. `HAL_Init()`
3. `HW_Init()`
Then queues/semaphores are created and tasks launched (`Task_Display_Create`, `Task_Net_Create`, `Task_Sign_Create`, `Task_IO_Create`, `Task_User_Create`).

Practical rationale for this order:
- LwIP branch requires MPU/cache before HAL so Ethernet DMA and LwIP heap work correctly.
- "OLED init" branch is tied to I2C: `ssd1306_Init()` runs immediately after `MX_I2C1_Init()`.
- UART and USB/RNG (per flags) are prepared in `HW_Init()` so early logging and crypto foundation are available before main task logic.

### Register/Parameter Level without Rewriting Cube
`hw_init` doesn't replace Cube; it makes targeted HAL adjustments:
- Clock/PLL and system dividers via `RCC_*` and `HAL_RCC_*`.
- MPU via `HAL_MPU_ConfigRegion`.
- Peripherals via HAL `HAL_GPIO_Init`, `HAL_I2C_Init`, `HAL_UART_Init`.
Pinmux details and MSP initialization for I2C/USART typically reside in `stm32h7xx_hal_msp.c` (e.g., I2C1 and USART3).

## Interrupts/Registers
`hw_init` is the unique place where system controllers (MPU/cache/clock pipeline) are explicitly modified via HAL/CMSIS:
- **MPU:** Regions (in LwIP branch) set access/attributes for ETH DMA descriptors and LwIP heap.
- **Cache:** I/D-cache enabled after MPU setup.
- **RCC/PWR:** PLL selection, D2 SRAM3 clock enable, divider configuration.

## Relations
- Uses: `main.c` (call sequence), `hw_init.h` (API), HAL (`stm32h7xx_hal.h` and sub-modules), FreeRTOS indirectly (via `Task_Display_Log`, but RTOS init itself doesn't happen here).
- I2C/OLED: `ssd1306.h`, `ssd1306_fonts.h`, descriptor `hi2c1`.
- UART logging: `UART_Log()` (exported in `hw_init.h` interface), descriptor `huart3`.
- USB: `usb_device.h` and `MX_USB_Device_Init()` when `USE_WEBUSB`.
- RNG/crypto: `RNG_HandleTypeDef` + `HAL_RNG_Init()` when `USE_CRYPTO_SIGN`.
- Build flags: `USE_LWIP`, `SKIP_OLED`, `USE_WEBUSB`, `USE_CRYPTO_SIGN`.
