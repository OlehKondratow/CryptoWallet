# CryptoWallet Documentation Index

## 📚 Main Documentation

### System Architecture & Implementation
- **[architecture.md](architecture.md)** - System architecture overview, task-based design, FreeRTOS scheduler
- **[main.md](main.md)** - Application entry point, IPC initialization, task creation

### Hardware Implementation
- **[task_display.md](task_display.md)** - OLED SSD1306 display task and UI rendering
- **[task_display_minimal.md](task_display_minimal.md)** - Minimal display for minimal-lwip builds
- **[task_io.md](task_io.md)** - LED control and status indicators
- **[task_user.md](task_user.md)** - User button input handling (PC13)
- **[task_net.md](task_net.md)** - Network task, LwIP/Ethernet, HTTP server
- **[app_ethernet.md](app_ethernet.md)** - Ethernet interface and DHCP
- **[time_service.md](time_service.md)** - SNTP time synchronization

### USB & Communication
- **[usb_device.md](usb_device.md)** - USB device initialization
- **[usb_webusb.md](usb_webusb.md)** - WebUSB vendor interface implementation
- **[usbd_conf_cw.md](usbd_conf_cw.md)** - USB device configuration (BSP)
- **[usbd_desc_cw.md](usbd_desc_cw.md)** - USB descriptors

### Low-Level Hardware
- **[stm32h7xx_hal_msp.md](stm32h7xx_hal_msp.md)** - HAL MSP callbacks
- **[stm32h7xx_it.md](stm32h7xx_it.md)** - Interrupt handlers
- **[stm32h7xx_it_systick.md](stm32h7xx_it_systick.md)** - SysTick for minimal-lwip
- **[stm32h7xx_it_usb.md](stm32h7xx_it_usb.md)** - USB OTG HS interrupt handler
- **[FAULT_HANDLING.md](FAULT_HANDLING.md)** - Cortex-M faults, UART dump, `Error_Handler_At`, FreeRTOS assert

### Configuration
- **[lwipopts.md](lwipopts.md)** - LwIP TCP/IP stack configuration
- **[ssd1306_conf.md](ssd1306_conf.md)** - Display driver configuration
- **[wallet_shared.md](wallet_shared.md)** - Shared data structures and IPC handles

### Cryptography & Security
See [crypto/README.md](crypto/README.md) for comprehensive cryptography documentation

## 📋 Quick References

- **[QUICK_REFERENCE.md](analysis/QUICK_REFERENCE.md)** - Build commands, troubleshooting, tips
- **[reference-code.md](reference-code.md)** - Auto-generated code reference

## 🔧 Documentation System

- **[documentation_autogeneration.md](documentation_autogeneration.md)** - Auto-generation scripts and Make targets

## 🔍 Technical Analysis

- **[VISUAL_SUMMARY.md](analysis/VISUAL_SUMMARY.md)** - Architecture visualizations and diagrams
- **[ARCHITECTURE_DETAILED.md](analysis/ARCHITECTURE_DETAILED.md)** - Detailed architecture comparison

## 🧪 Testing & Validation

- **[TESTING_GUIDE_RNG_SIGNING.md](TESTING_GUIDE_RNG_SIGNING.md)** - Complete testing guide
- **[TEST_SCRIPTS_README.md](TEST_SCRIPTS_README.md)** - Python test framework reference

---

## 🇷🇺 Русский

Все файлы доступны с суффиксами `_ru.md`:
- Например: [architecture_ru.md](architecture_ru.md)

## 🇵🇱 Polski

Все файлы доступны с суффиксами `_pl.md`:
- Например: [architecture_pl.md](architecture_pl.md)

---

**Last Updated:** 2026-03-20
