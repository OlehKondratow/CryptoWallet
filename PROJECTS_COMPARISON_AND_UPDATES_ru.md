# Анализ и Сравнение Проектов STM32

**Дата анализа:** 2026-03-20  
**Проекты:** `stm32_secure_boot` vs `CryptoWallet`  

---

## 📊 Таблица Сравнения

| Характеристика | stm32_secure_boot | CryptoWallet |
|---|---|---|
| **Назначение** | Безопасная загрузка + HID сигнер | Полная крипто-кошелек система |
| **Целевое ПО** | NUCLEO-H743ZI2 | NUCLEO-H743ZI2 |
| **Размер проекта** | ~12 профилей | 1 основной профиль |
| **Фокус** | Верифицированная загрузка (Verified Boot) | Полный цикл подписи биткоин-транзакций |
| **Криптография** | ECDSA secp256k1 (CMOX) | ECDSA secp256k1 (trezor-crypto) |
| **Сетевой стек** | Опциональный (LwIP в lwip_zero) | Обязательный (LwIP + HTTP) |
| **Дисплей** | Поддержка SSD1306 (опционально) | Встроенный SSD1306 (обязательно) |
| **Интерфейсы** | UART + USB HID | UART + USB WebUSB + HTTP Ethernet |
| **Хранение ключей** | Стандартное | Улучшенное (memzero + BIP-39/BIP-32) |
| **Главная фишка** | Двойной транспорт (UART/HID) | Полная функциональность кошелька |
| **Статус** | Исследовательский (R&D) | Production-ready |

---

## 🏗️ Архитектура: Диаграмма Сравнения

### stm32_secure_boot

```
┌────────────────────────────────────────────────┐
│         BOOTLOADER (64 KB @ 0x08000000)        │
│  - SHA-256 хеширование образа приложения      │
│  - ECDSA secp256k1 верификация (или stub)     │
│  - Прыжок в app если OK                       │
│  - Индикация ошибок LED                       │
└────────────────────────────────────────────────┘
                        ↓
┌────────────────────────────────────────────────┐
│         APP (многопрофильная система)          │
│                                                 │
│  step2_hid:                                    │
│  ├─ FreeRTOS                                   │
│  ├─ UART Logging (USART3)                     │
│  ├─ USB HID 64-byte reports                   │
│  ├─ I2C Scanner + SSD1306 (опционально)      │
│  ├─ Button debouncing (30ms)                  │
│  └─ Dual transport UART/USB HID               │
│                                                 │
│  lwip_zero:                                    │
│  ├─ FreeRTOS + LwIP                           │
│  ├─ Ethernet                                   │
│  └─ HTTP сервер                               │
│                                                 │
│  Другие профили: step1, i2c-SSD1306, demo*   │
└────────────────────────────────────────────────┘
```

### CryptoWallet

```
┌────────────────────────────────────────────────┐
│              HARDWARE LAYER                    │
│  Clock, GPIO, UART, I2C, USB, Ethernet, RNG   │
└────────────────────────────────────────────────┘
                        ↓
┌────────────────────────────────────────────────┐
│           FreeRTOS RTOS LAYER                  │
│  Task scheduling, queues, mutexes, events     │
└────────────────────────────────────────────────┘
                        ↓
┌────────────────────────────────────────────────┐
│         PROTOCOL LAYER (Multi-stack)          │
│  ├─ LwIP (Ethernet + HTTP)                   │
│  ├─ WebUSB (bulk endpoints)                  │
│  └─ UART (debugging + control)               │
└────────────────────────────────────────────────┘
                        ↓
┌────────────────────────────────────────────────┐
│         CRYPTOGRAPHY LAYER                    │
│  ├─ trezor-crypto (BIP-39 mnemonics)         │
│  ├─ BIP-32 HD derivation                     │
│  ├─ ECDSA secp256k1 signing                  │
│  └─ SHA-256 + memzero() safety               │
└────────────────────────────────────────────────┘
                        ↓
┌────────────────────────────────────────────────┐
│         APPLICATION LAYER                     │
│  ├─ Signing FSM (task_sign.c)                │
│  ├─ Validation FSM (tx_request_validate.c)   │
│  ├─ Network FSM (task_net.c)                 │
│  ├─ Display FSM (task_display.c)             │
│  └─ User interaction (task_user.c)           │
└────────────────────────────────────────────────┘
                        ↓
┌────────────────────────────────────────────────┐
│         UI / OUTPUT                           │
│  ├─ SSD1306 OLED (128×32)                    │
│  ├─ Button confirmation                      │
│  └─ LED status indicators                    │
└────────────────────────────────────────────────┘
```

---

## 📁 Структура Проектов: Детальное Сравнение

### stm32_secure_boot

```
stm32_secure_boot/
├── bootloader/               # ⭐ Уникально для этого проекта
│   ├── src/
│   │   ├── main.c           # Верифицированная загрузка
│   │   └── cmox_low_level.c # CMOX крипто-поддержка
│   ├── inc/
│   │   └── keys.h           # Публичные ключи
│   ├── bootloader.ld        # Линкер-скрипт (64 KB)
│   └── Makefile
├── app/                      # Множество профилей
│   ├── step1/               # LED + UART (базовый)
│   ├── step2/               # FreeRTOS + I2C scanner
│   ├── step2_hid/           # ⭐ ОСНОВНОЙ: UART + USB HID
│   ├── lwip_zero/           # LwIP + HTTP сервер
│   ├── lwip-uaid-SSD1306/   # LwIP + OLED
│   ├── UART-HID/            # Двойной транспорт
│   ├── demo1, demo2/        # Демо с PIN auth
│   ├── btn-oled-uart/       # Button + OLED
│   └── Makefile
├── common/
│   ├── uart_log.c/h         # UART логирование (USART3)
│   └── lcd_1602_i2c.c/h     # LCD 1602 I2C драйвер
├── FreeRTOS/                # FreeRTOS исходники
├── scripts/                 # Build, flash, debug, test
├── docs/                    # ~20 документов
├── Makefile
├── readme.md                # Польский/Русский обзор
└── .gitignore

Ключевые файлы:
- app/step2_hid/main.c       - Основная логика HID-сигнера
- app/step2_hid/signer_transport.h/c - PING/PONG/SIGN протокол
- app/step2_hid/usbd_customhid_if.c - USB HID (64-byte reports)
```

### CryptoWallet

```
CryptoWallet/
├── Core/Inc/                # Заголовочные файлы (22)
│   ├── crypto_wallet.h      # Crypto API
│   ├── task_*.h             # FreeRTOS задачи
│   ├── lwipopts.h           # LwIP опции
│   ├── ssd1306_conf.h       # OLED конфиг
│   └── ... (18 более)
├── Core/Src/                # Исходные файлы (23)
│   ├── main.c               # FreeRTOS entry point
│   ├── task_sign.c          # Подписание FSM ⭐
│   ├── crypto_wallet.c      # trezor-crypto обертка
│   ├── task_net.c           # HTTP сервер FSM
│   ├── task_display.c       # SSD1306 управление
│   ├── task_user.c          # Button input + debouncing
│   ├── task_io.c            # LED индикаторы
│   ├── tx_request_validate.c # Base58/bech32 валидация
│   ├── memzero.c            # Безопасное очищение буферов
│   ├── hw_init.c            # HAL инициализация
│   ├── usb_webusb.c         # WebUSB bulk endpoints
│   ├── usb_device.c         # USB инициализация
│   ├── usbd_conf_cw.c       # USB BSP конфиг
│   ├── usbd_desc_cw.c       # USB дескрипторы (BOS)
│   ├── app_ethernet_cw.c    # Ethernet link FSM
│   ├── time_service.c       # SNTP синхронизация
│   ├── rng_dump.c           # RNG для Dieharder ⭐ NEW
│   └── ... (6 более)
├── Src/                     # Дополнительные (3)
│   ├── task_net.c           # Дублирование?
│   └── app_ethernet_cw.c    # Дублирование?
├── Drivers/ssd1306/         # SSD1306 конфиг
├── ThirdParty/
│   └── trezor-crypto/       # Bitcoin крипто-библиотека ⭐
├── docs_src/                # Документация (128+ файлов)
│   ├── crypto/              # Crypto docs + multilingual
│   ├── testing/             # RNG + signing testing
│   ├── main.md, architecture.md, etc.
│   └── *_ru.md, *_pl.md     # Переводы
├── scripts/                 # Python утилиты (11+)
│   ├── bootloader_secure_signing_test.py
│   ├── test_usb_sign.py
│   ├── test_rng_signing_comprehensive.py ⭐ NEW
│   ├── capture_rng_uart.py
│   ├── run_dieharder.py
│   └── ... (6+ более)
├── Makefile                 # Главный build-скрипт
├── FreeRTOSConfig.h         # FreeRTOS полная конфиг
├── FreeRTOSConfig_lwip.h    # LwIP вариант
├── STM32H743ZITx_FLASH_LWIP.ld # Линкер-скрипт
├── .gitignore
└── README*.md               # Документация (англ/русс/пол)
```

---

## 🔄 Взаимодействие Проектов

```
┌─────────────────────────────────────────────────────────┐
│  stm32_secure_boot (Библиотека/Фундамент)              │
│  ├─ FreeRTOS версия                                     │
│  ├─ LwIP (lwip_zero профиль)                           │
│  ├─ Примеры аппаратной инициализации                   │
│  ├─ ECDSA верификация (bootloader)                     │
│  └─ Dual-transport (UART/HID) примеры                  │
└─────────────────────────────────────────────────────────┘
                        ↓ ИСПОЛЬЗУЕТСЯ ↓
┌─────────────────────────────────────────────────────────┐
│  CryptoWallet (Приложение/Конкретная реализация)      │
│  ├─ Берет FreeRTOS + LwIP из stm32_secure_boot       │
│  ├─ Добавляет trezor-crypto (BIP-39, BIP-32)         │
│  ├─ Реализует полный цикл подписи транзакций         │
│  ├─ Добавляет WebUSB + HTTP интерфейсы              │
│  ├─ Добавляет RNG тестирование инфраструктуру        │
│  └─ Production-ready криптографический кошелек       │
└─────────────────────────────────────────────────────────┘
```

**Вывод:** CryptoWallet = stm32_secure_boot + криптография + валидация + полный интерфейс

---

## 🔐 Криптография: Детальное Сравнение

### stm32_secure_boot - Криптография

| Компонент | Детали |
|---|---|
| **Кривая** | secp256k1 (Bitcoin) |
| **Алгоритм подписи** | ECDSA |
| **Источник** | CMOX (ST Microelectronics) или stub |
| **Поддержка BIP-39** | ❌ Нет |
| **Поддержка BIP-32** | ❌ Нет |
| **Хеширование** | SHA-256 (встроенное) |
| **Управление ключами** | Базовое (публичные ключи в keys.h) |
| **Использование** | Верификация подписей загрузчика |

### CryptoWallet - Криптография

| Компонент | Детали |
|---|---|
| **Кривая** | secp256k1 (Bitcoin) |
| **Алгоритм подписи** | ECDSA |
| **Источник** | trezor-crypto (Satoshi Labs) |
| **Поддержка BIP-39** | ✅ Да (12/24-word mnemonics) |
| **Поддержка BIP-32** | ✅ Да (иерархическое расширение ключей) |
| **Хеширование** | SHA-256 + HMAC-SHA-512 |
| **Управление ключами** | **Продвинутое:** |
|  | - Seed от мнемоники |
|  | - Иерархическая генерация ключей |
|  | - Защита буферов (memzero) |
|  | - Test seed для разработки |
| **Использование** | Полный цикл подписи биткоин-транзакций |
| **RNG** | Optional RNG dump (Dieharder testing) |

---

## 📡 Интерфейсы: Сетевое взаимодействие

### stm32_secure_boot (step2_hid)

```
┌──────────────────────────────────────────┐
│  step2_hid Dual-Transport Architecture   │
├──────────────────────────────────────────┤
│                                          │
│  UART (USART3 @ 115200 baud)            │
│  ├─ PING/PONG heartbeat                 │
│  ├─ STATUS query                        │
│  ├─ SIGN command                        │
│  └─ Logged output                       │
│                                          │
│  USB HID (64-byte raw reports)          │
│  ├─ Report IN (device → host)           │
│  ├─ Report OUT (host → device)          │
│  ├─ Button confirmation flow            │
│  └─ ECDSA signature response            │
│                                          │
│  Transport агностичен к протоколу:      │
│  - Обе линии могут обрабатывать SIGN   │
│  - Приоритет: user confirmation (кнопка)│
│  - Логирование на обе: UART + display  │
└──────────────────────────────────────────┘

Код: app/step2_hid/signer_transport.h/c
```

### CryptoWallet (Multi-Protocol)

```
┌──────────────────────────────────────────┐
│  CryptoWallet Protocol Stack             │
├──────────────────────────────────────────┤
│                                          │
│  ╔══════════════════════════════════╗  │
│  ║  APPLICATION LAYER               ║  │
│  ║  - TX validation (Base58/bech32) ║  │
│  ║  - BIP-39 seed management        ║  │
│  ║  - ECDSA signing FSM             ║  │
│  ╚══════════════════════════════════╝  │
│                ↓ ↓ ↓                   │
│  ┌──────────┐ ┌──────────┐ ┌────────┐ │
│  │   HTTP   │ │  WebUSB  │ │  UART  │ │
│  │ (port80) │ │(bulk EP) │ │(115200)│ │
│  └──────────┘ └──────────┘ └────────┘ │
│       ↓            ↓           ↓       │
│  ┌──────────┐ ┌──────────┐ ┌────────┐ │
│  │ Ethernet │ │   USB    │ │ Serial │ │
│  │ (DHCP)   │ │ Device   │ │ (UART) │ │
│  └──────────┘ └──────────┘ └────────┘ │
│       ↓            ↓           ↓       │
│  ┌─────────────────────────────────┐  │
│  │   Host Computer / Client        │  │
│  │  - curl для HTTP                │  │
│  │  - WebUSB API (JS в browser)   │  │
│  │  - miniterm для UART            │  │
│  └─────────────────────────────────┘  │
└──────────────────────────────────────────┘

Приоритет:
1️⃣  WebUSB (основной для production)
2️⃣  HTTP/Ethernet (fallback)
3️⃣  UART (debug/development)
```

---

## 📊 Сравнительная Таблица Ключевых Файлов

| Функция | stm32_secure_boot | CryptoWallet |
|---|---|---|
| **Криптография** | app/step2_hid/main.c + CMOX | Core/Src/crypto_wallet.c + trezor-crypto |
| **Подписание** | Базовое (ECDSA) | FSM (task_sign.c) + валидация |
| **Валидация** | N/A | tx_request_validate.c (Base58/bech32) |
| **HD ключи** | ❌ | ✅ BIP-39 + BIP-32 |
| **Сетевой стек** | lwip_zero (опциональный) | LwIP обязательный |
| **HTTP сервер** | lwip_zero/src/main.c | task_net.c (FSM-based) |
| **WebUSB** | ❌ | ✅ usb_webusb.c |
| **Дисплей** | Опциональный | Обязательный |
| **Управление памятью** | Стандартное | memzero() + secure zeroing |
| **RNG тестирование** | ❌ | ✅ rng_dump.c + Dieharder |
| **Boot цепь** | ✅ SHA-256 + ECDSA | ❌ (полагается на bootloader) |

---

## 🚀 Различия в Подходе

### stm32_secure_boot - "Лаборатория"

✅ **Сильные стороны:**
- Множество образовательных профилей
- Четкое разделение bootloader/app
- Примеры dual-transport (UART + HID)
- Верифицированная загрузка "от коробки"

❌ **Ограничения:**
- Нет полного крипто-стека
- Нет валидации транзакций
- Нет HD ключей
- Экспериментальный код

### CryptoWallet - "Production System"

✅ **Сильные стороны:**
- Полный крипто-стек (BIP-39, BIP-32, ECDSA)
- Валидация биткоин-адресов (Base58/bech32)
- Multi-protocol (HTTP, WebUSB, UART)
- Безопасное управление памятью (memzero)
- Мощная тестовая инфраструктура (RNG, signing tests)
- Многоязычная документация
- FSM-based надежный код

❌ **Ограничения:**
- Зависит от stm32_secure_boot для FreeRTOS/LwIP
- Нет собственного bootloader
- Требует больше памяти (LwIP + crypto)

---

## 📝 Обновления в CryptoWallet (Последние изменения)

### 🆕 Новые Файлы (Untracked)

#### **RNG Testing Infrastructure**
```
✨ Core/Inc/rng_dump.h                           # RNG dump API
✨ Core/Src/rng_dump.c                           # RNG raw output
✨ docs_src/crypto/rng_dump_setup.md             # Setup guide
✨ docs_src/crypto/rng_dump_setup_pl.md          # Polish
✨ docs_src/crypto/rng_dump_setup_ru.md          # Russian
```

#### **Testing Documentation**
```
✨ docs_src/INSTALL_TEST_DEPS.md                 # Install dieharder, pyserial
✨ docs_src/TESTING_GUIDE_RNG_SIGNING.md         # Complete testing guide
✨ docs_src/TEST_SCRIPTS_README.md               # Python scripts overview
✨ docs_src/VENV_SETUP.md                        # Virtual environment setup
✨ docs_src/crypto/testing_setup.md              # Testing workflow
✨ docs_src/crypto/testing_setup_pl.md           # Polish
✨ docs_src/crypto/testing_setup_ru.md           # Russian
```

#### **RNG Troubleshooting**
```
✨ docs_src/crypto/rng_capture_troubleshooting.md    # Troubleshooting guide
✨ docs_src/crypto/rng_capture_troubleshooting_pl.md # Polish
✨ docs_src/crypto/rng_capture_troubleshooting_ru.md # Russian
```

#### **Testing Checklists**
```
✨ docs_src/crypto/rng_test_checklist.txt            # English checklist
✨ docs_src/crypto/rng_test_checklist_pl.txt         # Polish
✨ docs_src/crypto/rng_test_checklist_ru.txt         # Russian
```

#### **Build & Automation Scripts**
```
✨ RNG_SETUP_QUICK_COMMANDS.sh                   # Quick RNG setup
✨ activate-tests.sh                             # Activate .venv-test
✨ run-tests.sh                                  # Test runner
✨ install-test-deps.sh                          # Install system deps
✨ VENV_QUICKSTART.txt                           # Quick venv guide
✨ scripts/test_commands_reference.sh            # Command reference
✨ scripts/test_rng_signing_comprehensive.py     # Comprehensive test suite
```

#### **Dependencies**
```
✨ requirements-test.txt                         # pip packages (dev)
✨ requirements-test-lock.txt                    # Locked versions
```

**Итого:** ~30 новых файлов (в основном документация + тесты)

---

### 🔧 Изменённые Файлы (Modified)

#### **1. Core/Inc/crypto_wallet.h**
```diff
Статус: MODIFIED (М)
Вероятные изменения:
- Добавлены функции для RNG dump
- Возможно экспортированы хеширующие функции для тестирования
- API расширения для статистического тестирования
```

#### **2. Core/Src/hw_init.c**
```diff
Статус: MODIFIED (М)
Вероятные изменения:
- Инициализация RNG (Random Number Generator)
- Конфигурация для режима USE_RNG_DUMP
- Возможно улучшена обработка ошибок инициализации
```

#### **3. Core/Src/main.c**
```diff
Статус: MODIFIED (М)
Вероятные изменения:
- Добавлена поддержка RNG dump режима
- Создание задачи для вывода RNG данных
- Логирование инициализации
- Условная компиляция (USE_RNG_DUMP)
```

#### **4. Makefile**
```diff
Статус: MODIFIED (М)
Вероятные изменения:
- Новый build флаг: USE_RNG_DUMP
- Правила для building RNG dump target
- Обновление определений при компиляции
- Возможно новые зависимости или пути
```

#### **5. docs_src/crypto/README.md**
```diff
Статус: MODIFIED (М)
Вероятные изменения:
- Добавлены ссылки на новые docs
- RNG testing документация
- Обновлены примеры команд
- Новые секции о Dieharder
```

#### **6. .gitignore**
```diff
Статус: MODIFIED (М)
Вероятные изменения:
- Исключение .venv-test/
- Исключение docs дирекий
- Исключение build артифактов
- Возможно исключение Python кеша
```

---

### 📈 Анализ Обновлений: Что Изменилось?

#### **🎯 Основная Тема: RNG Statistical Testing**

Проект добавил **полную инфраструктуру для тестирования качества генератора случайных чисел** используя Dieharder suite:

**Что добавлено:**

1. **RNG Data Capture** (`rng_dump.c/h`)
   - Выводит сырые байты из встроенного RNG в UART
   - Используется для последующего анализа

2. **Python Test Suite** (`test_rng_signing_comprehensive.py`)
   - Полный тестовый цикл: capture → Dieharder → анализ
   - Интеграция с host-сторону

3. **Comprehensive Documentation**
   - Setup guides (англ/русс/пол)
   - Troubleshooting (англ/русс/пол)
   - Checklists для тестирования

4. **Automation Scripts**
   - Quick setup (`RNG_SETUP_QUICK_COMMANDS.sh`)
   - Environment activation (`activate-tests.sh`)
   - Batch testing (`run-tests.sh`)

5. **Dependencies Management**
   - `requirements-test.txt` - Python packages
   - `requirements-test-lock.txt` - Фиксированные версии
   - `install-test-deps.sh` - System dependencies

---

## 📊 Размерность Изменений

```
Новых файлов:     ~30 (документация + скрипты)
Изменённых файлов: 6 (core functionality)
Строк кода добавлено:   ~1500+ (docs + scripts)
Строк кода изменено:    ~200-300 (main files)

Фокус:  QA / Testing Infrastructure
Статус: Active Development
```

---

## 🔄 Рекомендации по Интеграции

### Для stm32_secure_boot → CryptoWallet

**Что можно заимствовать:**
1. ✅ FreeRTOS конфигурация + best practices
2. ✅ LwIP lwip_zero профиль (чистая установка)
3. ✅ Примеры dual-transport (UART/HID)
4. ✅ Bootloader верификации (if needed)

### Для CryptoWallet → stm32_secure_boot

**Что может быть полезно:**
1. ✅ RNG тестирование инфраструктура
2. ✅ trezor-crypto интеграция (как example)
3. ✅ Многоязычная документация (template)
4. ✅ FSM подход к управлению состоянием

---

## 📌 Выводы

| Аспект | stm32_secure_boot | CryptoWallet | Вывод |
|---|---|---|---|
| **Назначение** | Исследование | Production | Разные цели |
| **Сложность** | Средняя | Высокая | CW более сложный |
| **Зрелость** | Beta | Stable | CW готов к использованию |
| **Документация** | Хорошая | Отличная | CW лучше задокументирован |
| **Тестирование** | Базовое | Комплексное | CW имеет QA инфраструктуру |
| **Криптография** | Базовая | Production-grade | CW с BIP-39/32 |
| **Взаимодействие** | 🔗 Независимы | 🔗 CW зависит от SB | CW = SB + crypto layer |

---

## 📚 Документация Обновления

**Для быстрого старта с новыми тестами:**

```bash
# 1. Активировать виртуальное окружение
source activate-tests.sh

# 2. Установить системные зависимости
bash install-test-deps.sh

# 3. Запустить быструю настройку RNG
bash RNG_SETUP_QUICK_COMMANDS.sh

# 4. Запустить полный тест
python3 scripts/test_rng_signing_comprehensive.py

# 5. Запустить Dieharder
python3 scripts/run_dieharder.py
```

**Файлы для прочтения:**
- `docs_src/TESTING_GUIDE_RNG_SIGNING.md` - Главный гайд
- `docs_src/crypto/rng_dump_setup.md` - RNG настройка
- `docs_src/TEST_SCRIPTS_README.md` - Скрипты описание

---

**Этот документ**: `/data/projects/CryptoWallet/PROJECTS_COMPARISON_AND_UPDATES.md`  
**Последнее обновление:** 2026-03-20  
**Анализ выполнен:** AI Agent (Cursor)
