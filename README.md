# CryptoWallet

## Project Structure

| Module | Brief |
|--------|-------|
| `Core/Inc/app_ethernet.h` | Ethernet link callback and DHCP thread interface. |
| `Core/Inc/crypto_wallet.h` | trezor-crypto integration: RNG, BIP-39, BIP-32, ECDSA secp256k1. |
| `Core/Inc/hw_init.h` | Hardware initialization wrapper (CMSIS-compliant). |
| `Core/Inc/lwipopts.h` | (no description) |
| `Core/Inc/main.h` | Board pins, LEDs, UART logging macro, network IP defaults. |
| `Core/Inc/memzero.h` | Secure memory zeroing (prevents compiler optimization). |
| `Core/Inc/sha256_minimal.h` | (no description) |
| `Core/Inc/task_display.h` | OLED task API — SSD1306 UI types and Task_Display_Log . |
| `Core/Inc/task_io.h` | IO module - LEDs only. |
| `Core/Inc/task_net.h` | LwIP + HTTP — Ethernet stack and port 80 API for transactions. |
| `Core/Inc/task_security.h` | Header for legacy task_security.c (mock crypto FSM). |
| `Core/Inc/task_sign.h` | Production signing task — g_tx_queue consumer, USER confirm, ECDSA. |
| `Core/Inc/task_user.h` | User button task — USER key (PC13) handling. |
| `Core/Inc/time_service.h` | SNTP API — init, start after link, epoch + formatted UTC. |
| `Core/Inc/tx_request_validate.h` | Request analysis and validation for crypto transaction signing. |
| `Core/Inc/usb_device.h` | USB device init for CryptoWallet WebUSB. |
| `Core/Inc/usb_webusb.h` | WebUSB vendor-specific class for CryptoWallet. |
| `Core/Inc/usbd_conf.h` | USB device conf - redirects to CryptoWallet WebUSB config. |
| `Core/Inc/usbd_conf_cw.h` | USB device BSP configuration for CryptoWallet WebUSB. |
| `Core/Inc/usbd_desc_cw.h` | USB device descriptors for CryptoWallet WebUSB. |
| `Core/Inc/wallet_shared.h` | Shared types and IPC: queues, events, mutexes, display context. |
| `Core/Src/crypto_wallet.c` | trezor-crypto glue: STM32 TRNG, random_buffer , BIP-32, ECDSA sign. |
| `Core/Src/hw_init.c` | Board bring-up: clock, MPU/cache, GPIO, I2C1 (OLED), UART, optional USB. |
| `Core/Src/main.c` | FreeRTOS entry: IPC objects, task creation, OS hooks. |
| `Core/Src/memzero.c` | Secure memzero() — volatile byte writes (no optimize-out). |
| `Core/Src/sha256_minimal.c` | SHA-256 only — used when USE_CRYPTO_SIGN=0 (no trezor-crypto). |
| `Core/Src/stm32h7xx_hal_msp.c` | MSP init for CryptoWallet - I2C1 (SSD1306). |
| `Core/Src/stm32h7xx_it.c` | Interrupt handlers - FreeRTOS SysTick, ETH (when USE_LWIP). |
| `Core/Src/stm32h7xx_it_systick.c` | SysTick handler for minimal-lwip (FreeRTOS tick). |
| `Core/Src/stm32h7xx_it_usb.c` | USB OTG HS interrupt handler (WebUSB). |
| `Core/Src/task_display.c` | SSD1306 128×32 — four scroll lines, state machine, queue-driven UI. |
| `Core/Src/task_display_minimal.c` | Reduced display task for minimal-lwip — faster Ethernet-first bring-up. |
| `Core/Src/task_io.c` | LED policy task — system / network / alert indicators only. |
| `Core/Src/task_security.c` | Alternate signing FSM with mock SHA256/ECDSA (placeholders). |
| `Core/Src/task_sign.c` | Primary signing task — consumes g_tx_queue , USER confirm, ECDSA. |
| `Core/Src/task_user.c` | Physical UX — USER (PC13) debounce, confirm vs reject for signing. |
| `Core/Src/time_service.c` | SNTP client — wall-clock epoch and UTC strings for logs/UI. |
| `Core/Src/tx_request_validate.c` | Validate host-supplied recipient / amount / currency before signing. |
| `Core/Src/usb_device.c` | USB device initialization for CryptoWallet WebUSB. |
| `Core/Src/usb_webusb.c` | WebUSB vendor class — ping/pong and binary sign request/response. |
| `Core/Src/usbd_conf_cw.c` | USB device BSP for CryptoWallet WebUSB (NUCLEO-H743ZI2, PA11/PA12). |
| `Core/Src/usbd_desc_cw.c` | USB device descriptors for CryptoWallet WebUSB. |
| `Core/Src/wallet_seed.c` | Strong get_wallet_seed() when USE_TEST_SEED=1 (development only). |
| `Drivers/ssd1306/ssd1306_conf.h` | Display driver tuning — I2C1, 128×32, 0x3C, Font 6×8. |
| `Src/app_ethernet_cw.c` | Ethernet glue — link callbacks, DHCP state machine, LED feedback. |
| `Src/task_net.c` | LwIP + HTTP — DHCP/static IP, POST /tx, signing poll endpoints. |

## Учебные разборы (docs_src)
<!-- DOXYGEN_DOCS_SRC_INDEX -->
| Модуль | Краткий обзор |
|--------|------------------|
| [api-documentation-scope](docs_src/api-documentation-scope.md) | Страница фиксирует прогресс по Doxygen-покрытию и показывает, какие модули уже получили “развёрнутый” комментарий (через `@brief/@details`). Следующий шаг — превратить оставшиеся места в такие же учебные разборы без перегруза исходников. |
| [doxygen-comments](docs_src/doxygen-comments.md) | У Doxygen нет нативной опции `GENERATE_MARKDOWN`, поэтому этот проект использует **XML output** и скрипты для генерации Markdown и обновления README. Чтобы корректно разделять **короткое** (`@brief`) и **длинное** (`@details`) описание, в коде применяется единый шаблон комментариев. |
| [hw_init](docs_src/hw_init.md) | Модуль `hw_init` отвечает за низкоуровневый “bring-up” платы: настройку системных тактов, (в зависимости от сборки) MPU/кэш для LwIP, GPIO для индикации/UX, инициализацию I2C1 (OLED/SSD1306) и USART3 (UART-лог), а также опциональные USB и RNG. Важная часть логики спрятана в правильном порядке вызовов между `main.c`, `HW_Init_Early_LwIP()` и `HW_Init()`. |
| [main](docs_src/main.md) | <brief>Модуль `main` описывает “склейку” всего приложения на STM32H7: он задаёт порядок ранних HAL/LwIP инициализаций, создаёт IPC-объекты (очереди/семафоры/event group), запускает критические задачи FreeRTOS и поднимает планировщик.</brief> |
| [task_display](docs_src/task_display.md) | <brief>Модуль `task_display` управляет визуальным состоянием кошелька на SSD1306: он принимает события сети/подписания, объединяет их в единое отображаемое состояние и рендерит 4 строки UI с “бегущей строкой” для логов и сетевых данных.</brief> |
| [task_display_minimal](docs_src/task_display_minimal.md) | <brief>Модуль `task_display_minimal` — облегчённая реализация UI/лога для `minimal-lwip`: он минимизирует нагрузку на SSD1306, зеркалит сообщения в UART и пишет короткий хвост в `g_display_ctx`, чтобы дисплей можно было обновлять только по необходимости.</brief> |
| [task_io](docs_src/task_io.md) | <brief>Модуль `task_io` отвечает за визуальные индикаторы безопасности и статуса системы: он периодически обновляет LED1 как “alive”, управляет LED2 как сетевым индикатором (в зависимости от сборки LwIP) и включает LED3 при наличии security alert.</brief> |
| [task_user](docs_src/task_user.md) | <brief>Модуль `task_user` реализует физическую UX-логику для кнопки USER (PC13): делает debounce, различает короткое нажатие (Confirm) и длинное удержание (~2.5s) как Reject и сигналит это в `task_sign` через `g_user_event_group`.</brief> |
| [time_service](docs_src/time_service.md) | <brief>Модуль `time_service` обеспечивает синхронизацию времени по SNTP и даёт приложению унифицированный доступ к текущему Unix epoch и строковому представлению UTC (для логов/UI), построенному поверх `HAL_GetTick()` после получения epoch из сети.</brief> |
<!-- /DOXYGEN_DOCS_SRC_INDEX -->
