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
| [app_ethernet](docs_src/app_ethernet.md) | <brief>Заголовок `app_ethernet` задаёт интерфейсы Ethernet “glue” слоя: он объявляет callback `ethernet_link_status_updated()`, функции чтения текущих LED-состояний и, при включённом `LWIP_DHCP`, объявляет `DHCP_Thread()` и набор констант состояний FSM DHCP, которые используются реализацией в `Src/app_ethernet_cw.c`.</brief> |
| [app_ethernet_cw](docs_src/app_ethernet_cw.md) | <brief>Модуль `app_ethernet_cw` — поддержка Ethernet: FSM для link-up/link-down состояний, DHCP клиент (START -> WAIT_ADDRESS -> ASSIGNED/TIMEOUT), LED feedback (LED2 = network, LED3 = link-down), логирование IP-адреса.</brief> |
| [crypto_wallet](docs_src/crypto_wallet.md) | <brief>Модуль `crypto_wallet` обёртывает trezor-crypto библиотеку: он поднимает STM32 RNG с энтропийным перемешиванием, предоставляет BIP-39 mnemonics, BIP-32 HDNode derivation и ECDSA secp256k1 подпись для стандартного пути Bitcoin m/44'/0'/0'/0/0.</brief> |
| [doxygen-comments](docs_src/doxygen-comments.md) | У Doxygen нет нативной опции `GENERATE_MARKDOWN`, поэтому этот проект использует **XML output** и скрипты для генерации Markdown и обновления README. Чтобы корректно разделять **короткое** (`@brief`) и **длинное** (`@details`) описание, в коде применяется единый шаблон комментариев. |
| [hw_init](docs_src/hw_init.md) | <brief>Модуль `hw_init` отвечает за низкоуровневый bring-up платы: он формирует правильный порядок тактов/кэша/MPU (критично для LwIP/ETH), инициализирует GPIO для UX (LED/кнопка), поднимает I2C1 под SSD1306 и UART-лог, а также опционально поднимает USB и RNG.</brief> |
| [lwipopts](docs_src/lwipopts.md) | <brief>Заголовок `lwipopts` задаёт compile-time параметры LwIP для проекта: включает IPv4/TCP/DHCP/DNS/SNTP, настраивает размер heap’а (`LWIP_RAM_HEAP_POINTER` и `MEM_SIZE`), параметры TCP буферов/окна, а также привязывает установку времени из SNTP к `time_service_set_epoch()`.</brief> |
| [main](docs_src/main.md) | <brief>Модуль `main` описывает “склейку” всего приложения на STM32H7: он задаёт порядок ранних HAL/LwIP инициализаций, создаёт IPC-объекты (очереди/семафоры/event group), запускает критические задачи FreeRTOS и поднимает планировщик.</brief> |
| [memzero](docs_src/memzero.md) | <brief>Модуль `memzero` вычищает sensitive buffer'ы (приватные ключи, digests, seeds) через volatile-writes, предотвращая оптимизацию компилятора и оставляя в памяти следы опасных данных.</brief> |
| [README](docs_src/README.md) | Здесь лежат **развёрнутые текстовые разборы** логики кода — дополнение к комментариям в исходниках и к выводу Doxygen (`make docs-doxygen`). |
| [sha256_minimal](docs_src/sha256_minimal.md) | <brief>Модуль `sha256_minimal` — компактная реализация SHA-256 (public domain), используется при `USE_CRYPTO_SIGN=0`, когда trezor-crypto не линкуется, но нужно хешировать данные для UI/логирования.</brief> |
| [ssd1306_conf](docs_src/ssd1306_conf.md) | <brief>Заголовок `ssd1306_conf` задаёт build-time конфигурацию для SSD1306 драйвера: привязывает I2C1 (`hi2c1`) и адрес 0x3C, устанавливает геометрию дисплея 128×32 и включает нужный шрифт (Font 6×8). Эти макросы потребляются исходниками драйвера и вызывающим UI-кодом.</brief> |
| [stm32h7xx_hal_msp](docs_src/stm32h7xx_hal_msp.md) | <brief>Файл `stm32h7xx_hal_msp` задаёт “железную” часть для HAL: `HAL_I2C_MspInit` конфигурирует I2C1 (PB8/PB9 под AF OD с подтяжкой), а `HAL_UART_MspInit` настраивает USART3 (TX/RX пины и клоки). Это гарантирует, что `hw_init` сможет инициализировать I2C/I2C-периферию и UART без ручного pin-парсинга в приложении.</brief> |
| [stm32h7xx_it](docs_src/stm32h7xx_it.md) | <brief>Файл `stm32h7xx_it` реализует критические обработчики прерываний для базового сценария: `SysTick_Handler` обеспечивает связку HAL tick с tick FreeRTOS, а при включённом LwIP обрабатывает Ethernet IRQ через `HAL_ETH_IRQHandler`.</brief> |
| [stm32h7xx_it_systick](docs_src/stm32h7xx_it_systick.md) | <brief>Файл `stm32h7xx_it_systick` корректирует ситуацию для сборки minimal-lwip: он предоставляет `SysTick_Handler`, который синхронизирует HAL tick и tick FreeRTOS, чтобы избежать зависаний/некорректного поведения, когда базовый `stm32h7xx_it.c` не содержит `SysTick_Handler`.</brief> |
| [stm32h7xx_it_usb](docs_src/stm32h7xx_it_usb.md) | <brief>Файл `stm32h7xx_it_usb` реализует обработчик прерывания OTG HS для режима WebUSB: в ISR он вызывает `HAL_PCD_IRQHandler` для `hpcd_USB_FS`, чтобы USB device middleware мог корректно обслуживать transfer’ы endpoint’ов.</brief> |
| [task_display](docs_src/task_display.md) | <brief>Модуль `task_display` управляет визуальным состоянием кошелька на SSD1306: он принимает события сети/подписания, объединяет их в единое отображаемое состояние и рендерит 4 строки UI с “бегущей строкой” для логов и сетевых данных.</brief> |
| [task_display_minimal](docs_src/task_display_minimal.md) | <brief>Модуль `task_display_minimal` — облегчённая реализация UI/лога для `minimal-lwip`: он минимизирует нагрузку на SSD1306, зеркалит сообщения в UART и пишет короткий хвост в `g_display_ctx`, чтобы дисплей можно было обновлять только по необходимости.</brief> |
| [task_io](docs_src/task_io.md) | <brief>Модуль `task_io` отвечает за визуальные индикаторы безопасности и статуса системы: он периодически обновляет LED1 как “alive”, управляет LED2 как сетевым индикатором (в зависимости от сборки LwIP) и включает LED3 при наличии security alert.</brief> |
| [task_net](docs_src/task_net.md) | <brief>Модуль `task_net` — сетевой фасад приложения: запускает LwIP/Ethernet, поднимает HTTP-сервер на порту 80, парсит JSON/form POST запросы (`POST /tx`), валидирует транзакции и отправляет их на подпись через `g_tx_queue`.</brief> |
| [task_security](docs_src/task_security.md) | <brief>Модуль `task_security` хранит “legacy/mock” вариант signing FSM для bring-up и сравнения: он не используется в основном пути (`main.c` не вызывает его), но реализует схожую автоматику подтверждения и подменяет криптографию на заглушки.</brief> |
| [task_sign](docs_src/task_sign.md) | <brief>Модуль `task_sign` реализует основной pipeline подписания: он получает валидированный запрос из очереди, формирует детерминированный input для SHA-256, ждёт пользовательское подтверждение/отклонение и, при успехе, сохраняет компактную подпись и подготавливает её для сетевого/USB ответа.</brief> |
| [task_user](docs_src/task_user.md) | <brief>Модуль `task_user` реализует физическую UX-логику для кнопки USER (PC13): делает debounce, различает короткое нажатие (Confirm) и длинное удержание (~2.5s) как Reject и сигналит это в `task_sign` через `g_user_event_group`.</brief> |
| [time_service](docs_src/time_service.md) | <brief>Модуль `time_service` обеспечивает синхронизацию времени по SNTP и даёт приложению унифицированный доступ к текущему Unix epoch и строковому представлению UTC (для логов/UI), построенному поверх `HAL_GetTick()` после получения epoch из сети.</brief> |
| [tx_request_validate](docs_src/tx_request_validate.md) | <brief>Модуль `tx_request_validate` — охранный шлюз перед подписанием: он проверяет host-supplied поля (адрес получателя, сумму, валюту) на базовое соответствие формату, помогая избежать очевидно неправильных данных на SSD1306/в логе и защищая signing pipeline от груд ошибок.</brief> |
| [usb_device](docs_src/usb_device.md) | <brief>Модуль `usb_device` поднимает USB устройство для WebUSB: он настраивает источник такта для USB (HSI48), и затем запускает USBD core, регистрирует класс WebUSB и стартует обработку USB событий.</brief> |
| [usb_webusb](docs_src/usb_webusb.md) | <brief>Модуль `usb_webusb` реализует vendor-specific WebUSB интерфейс: он открывает bulk endpoints, поддерживает команду `ping` (→ `pong`) и принимает бинарно-фреймленный запрос подписи, извлекает `recipient/amount/currency`, валидирует их и ставит транзакцию в `g_tx_queue`; при готовности подписи отправляет 64-байтовый компактный `r\|\|s` через `WebUSB_NotifySignatureReady()`.</brief> |
| [usbd_conf](docs_src/usbd_conf.md) | <brief>Заголовок `usbd_conf` является тонкой обёрткой: он подключает `usbd_conf_cw.h`, тем самым “привязывая” параметры USB device middleware к конкретной реализации CryptoWallet WebUSB BSP.</brief> |
| [usbd_conf_cw](docs_src/usbd_conf_cw.md) | <brief>Модуль `usbd_conf_cw` — это BSP/конфигурация для USB device middleware: он задаёт статический аллокатор для USBD, описывает memory hooks и настраивает MSP для PCD (GPIO alternate function, enable clock, NVIC priority/enable), а также мостит события HAL_PCD callback’ами в USBD_LL.</brief> |
| [usbd_desc_cw](docs_src/usbd_desc_cw.md) | <brief>Модуль `usbd_desc_cw` формирует USB дескрипторы для устройства WebUSB: device/interface/BOS и строки (manufacturer/product/serial), включая WebUSB Platform Capability UUID, а также динамическую генерацию серийного номера на основе DEVICE_ID регистров STM32.</brief> |
| [wallet_seed](docs_src/wallet_seed.md) | <brief>Модуль `wallet_seed` — заглушка для `get_wallet_seed()` при `USE_TEST_SEED=1`: возвращает BIP-39 seed из известного тестового мnemonic'a "abandon...about", **никогда не для реальных средств**.</brief> |
| [wallet_shared](docs_src/wallet_shared.md) | <brief>Заголовок `wallet_shared` задаёт единый контракт между модулями: структуры данных для “запрос → подтверждение → подпись” и UI-состояния, а также global IPC-ручки (очереди, event group, mutex’ы), которыми обмениваются `task_net`, `task_sign`, `task_user` и `task_display`.</brief> |
<!-- /DOXYGEN_DOCS_SRC_INDEX -->
