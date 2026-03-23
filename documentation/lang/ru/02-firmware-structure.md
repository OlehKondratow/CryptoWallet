# 2. Структура прошивки

## 2.1 МК и раскладка

- **МК:** STM32H743ZI (Cortex-M7), целевая плата NUCLEO-H743ZI2.
- **Flash:** приложение линкуется для сборки с LwIP в `STM32H743ZITx_FLASH_LWIP.ld` (см. `LDSCRIPT` в Makefile).
- **ОС:** FreeRTOS (в ряде мест API CMSIS-RTOS2).

## 2.2 Инициализация

- **`Core/Src/main.c`** — тактирование, старт RTOS, создание задач и примитивов IPC.
- **`Core/Src/hw_init.c`** — такты, правила MPU/кэша для DMA Ethernet, GPIO, I2C (OLED), UART, USB, периферия RNG согласно флагам сборки.

## 2.3 Задачи (концептуально)

| Область | Типичные файлы | Роль |
|---------|----------------|------|
| Дисплей | `task_display.c`, `task_display_minimal.c` | OLED, сводка по транзакции |
| Сеть | `Src/task_net.c` | HTTP-сервер, разбор запросов, очереди к подписи |
| Подпись | `task_sign.c` | Проверка запроса, UI, ECDSA, ответ |
| Пользователь / IO | `task_user.c`, `task_io.c` | Дребезг кнопки, светодиоды |
| USB | `usb_device.c`, `usb_webusb.c`, `usbd_*_cw.c` | WebUSB при `USE_WEBUSB=1` |
| UART | `cwup_uart.c`, `rng_dump.c` | CWUP или сырой поток RNG (взаимоисключающе на одном UART) |
| Целостность | `fw_integrity.c` | Старт + опционально `AT+FWINFO?` |

Устаревшие или аудиторские пути могут включать `task_security.c` — смотрите исходники перед опорой на них в продукте.

## 2.4 IPC и общие типы

- **`Core/Inc/wallet_shared.h`** — типы транзакций, дескрипторы очередей, контекст дисплея, лимиты (`TX_*_LEN`).
- **Очереди** — например сеть → подпись → дисплей; размеры и политика отбрасывания — часть корректности под нагрузкой (см. исходники).

## 2.5 Карта модулей (истина в коде)

| Задача | Основные файлы |
|--------|----------------|
| HTTP-маршруты | `Src/task_net.c` |
| Валидация запросов | `Core/Src/tx_request_validate.c` |
| Обёртка крипто | `Core/Src/crypto_wallet.c` |
| Безопасное обнуление | `Core/Src/memzero.c` |
| Ethernet / DHCP | `Src/app_ethernet_cw.c` |
| Время | `Core/Src/time_service.c` |
| Сбои | `Core/Src/fault_report.c`, `stm32h7xx_it*.c` |

## 2.6 Сторонние компоненты

- **`ThirdParty/trezor-crypto`** — BIP-39/32, secp256k1, хэши (при `USE_CRYPTO_SIGN=1`).
- **STM32 HAL / CMSIS** — через дерево `STM32CubeH7` (`CRYPTO_DEPS_ROOT` или соседний checkout).
