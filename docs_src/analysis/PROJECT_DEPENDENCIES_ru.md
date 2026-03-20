# Зависимости и Отношения Проектов

**Понимание того, как stm32_secure_boot и CryptoWallet связаны**

---

## 🔗 Граф Зависимостей

```
Внешние Зависимости (Сторонние):
├─ STM32CubeH7/                    (HAL ST Microelectronics)
│  └─ Используется: [оба проекта]
│
├─ STM32CubeExpansion_Crypto/      (Библиотека криптографии CMOX)
│  └─ Используется: stm32_secure_boot (загрузчик)
│
├─ Ядро FreeRTOS                   (RTOS)
│  ├─ Версия в stm32_secure_boot/FreeRTOS/
│  └─ Используется: [оба проекта через stm32_secure_boot]
│
├─ Стек сети LwIP                  (Стек TCP/IP)
│  ├─ Версия в stm32_secure_boot/ (профиль lwip_zero)
│  └─ Используется: CryptoWallet (обязательно)
│
└─ trezor-crypto/                  (Библиотека Bitcoin)
   ├─ Расположение: CryptoWallet/ThirdParty/
   └─ Используется: ТОЛЬКО CryptoWallet

Внутренние Зависимости:
stm32_secure_boot/
├─ Независимый
├─ Может быть построен отдельно
├─ Предоставляет загрузчик + примеры приложений
└─ Используется как справка CryptoWallet

            ↓ (ИСПОЛЬЗУЕТСЯ КАК ОСНОВАНИЕ)

CryptoWallet/
├─ Зависит от FreeRTOS из stm32_secure_boot
├─ Зависит от LwIP (lwip_zero)
├─ Зависит от драйверов HAL
└─ Добавляет: trezor-crypto, HD кошельки, полный стек подписания
```

---

## 📦 Дерево Зависимостей Компонентов

### stm32_secure_boot (Независимый)

```
┌─ stm32_secure_boot (АВТОНОМНЫЙ)
│
├─ bootloader/ ──┬─ Ядро загрузчика
│                ├─ SHA-256 (встроенный)
│                ├─ ECDSA (CMOX или заглушка)
│                └─ Хранилище ключей
│
└─ app/ ─────────┬─ step2_hid/ (ОСНОВНОЙ)
                 │  ├─ FreeRTOS
                 │  ├─ UART + USB HID
                 │  ├─ Кнопка + OLED
                 │  └─ Транспорт подписи
                 │
                 ├─ lwip_zero/
                 │  ├─ FreeRTOS
                 │  ├─ LwIP
                 │  └─ HTTP сервер
                 │
                 └─ step1, step2, и т.д.
```

### CryptoWallet (Композитный)

```
┌─ CryptoWallet (ПРОИЗВОДНЫЙ)
│
├─ Зависимости:
│  ├─ FreeRTOS (из stm32_secure_boot)
│  ├─ LwIP (из stm32_secure_boot/lwip_zero)
│  ├─ STM32CubeH7 (HAL)
│  └─ trezor-crypto (ThirdParty/)
│
├─ Основные функции:
│  ├─ Архитектура на основе задач (FreeRTOS)
│  ├─ Мультипротокол (LwIP + WebUSB + UART)
│  ├─ HD кошелек (BIP-39/32)
│  ├─ Валидация TX
│  ├─ Конечный автомат подписания
│  └─ Тестирование RNG ✨
│
└─ НЕ включает загрузчик
   (Может наследоваться из stm32_secure_boot при необходимости)
```

---

## 📊 Матрица Совместного Использования Кода и Повторного Использования

| Компонент | stm32_secure_boot | CryptoWallet | Заметки |
|---|---|---|---|
| **FreeRTOS** | ✅ Включены | ✅ Использует версию SB | Общее ядро |
| **LwIP** | ✅ lwip_zero | ✅ Использует версию SB | Стек сети |
| **STM32 HAL** | ✅ Внешний | ✅ Внешний | Оба используют одинаковый HAL |
| **Драйверы UART/USB/I2C** | ✅ Включены | ✅ Использует/расширяет SB | Частично общее |
| **Загрузчик** | ✅ Полный | ❌ Опциональный | Преимущество SB |
| **Криптография ECDSA** | ✅ CMOX | ✅ trezor-crypto | Различные библиотеки |
| **HD кошелек** | ❌ Нет | ✅ Да | Специфично для CW |
| **HTTP сервер** | ✅ lwip_zero | ✅ Улучшенный | CW улучшен |
| **WebUSB** | ❌ Нет | ✅ Да | Специфично для CW |
| **Тестирование RNG** | ❌ Нет | ✅ Да | Специфично для CW ✨ |

---

## 🔄 Интеграция Системы Сборки

### Как Собрать

#### stm32_secure_boot Автономно

```bash
cd /data/projects/stm32_secure_boot

# Загрузчик
make bootloader
arm-none-eabi-objcopy -O binary build/bootloader.elf build/bootloader.bin

# Приложение (step2_hid)
make step2_hid
arm-none-eabi-objcopy -O binary build/app/app.elf build/app/app.bin

# Прошивка
st-flash write build/bootloader.bin 0x08000000
st-flash write build/app/app.bin 0x08010000
```

#### CryptoWallet с Опциональным Загрузчиком

```bash
cd /data/projects/CryptoWallet

# Вариант 1: Собрать с загрузчиком stm32_secure_boot (рекомендуется)
cd /data/projects/stm32_secure_boot && make bootloader
cp build/bootloader.bin ../CryptoWallet/build/

# Вариант 2: Собрать только CryptoWallet (только приложение)
cd /data/projects/CryptoWallet
make
arm-none-eabi-objcopy -O binary build/cryptowallet.elf build/cryptowallet.bin

# Прошивка (CryptoWallet по адресу приложения)
st-flash write build/cryptowallet.bin 0x08010000

# Или прошивка с загрузчиком
st-flash write build/bootloader.bin 0x08000000
```

---

## 🏗️ Расположение Памяти с Обоими Проектами

### Если Использовать Загрузчик stm32_secure_boot + Приложение CryptoWallet

```
Расположение памяти Flash:
0x08000000 ┌──────────────────────────────────┐
           │  ЗАГРУЗЧИК (из stm32_sb)         │  64 KB
           │  ├─ Код загрузчика              │
           │  ├─ Проверка SHA-256            │
           │  ├─ Проверка ECDSA (CMOX)       │
           │  ├─ Открытые ключи              │
           │  └─ Отчёты об ошибках LED/UART │
0x08010000 ├──────────────────────────────────┤
           │  ПРИЛОЖЕНИЕ (CryptoWallet)      │  1.5 MB
           │  ├─ Ядро FreeRTOS              │
           │  ├─ Стек LwIP                  │
           │  ├─ trezor-crypto (Bitcoin)    │
           │  ├─ HTTP сервер                │
           │  ├─ Интерфейс WebUSB           │
           │  ├─ Менеджеры задач            │
           │  │  ├─ task_sign.c             │
           │  │  ├─ task_net.c              │
           │  │  ├─ task_display.c          │
           │  │  ├─ task_user.c             │
           │  │  └─ task_io.c               │
           │  ├─ Тестирование RNG (НОВОЕ)   │
           │  └─ Драйвер OLED (SSD1306)     │
0x08180000 ├──────────────────────────────────┤
           │  [Свободное пространство]      │  512 KB
           │                                 │
0x08200000 └──────────────────────────────────┘

Расположение памяти RAM:
0x20000000 ┌──────────────────────────────────┐
           │  Ядро FreeRTOS                 │
           │  ├─ Блоки управления задачами  │  100 KB
           │  ├─ Готовые списки             │
           │  └─ Структуры очереди/события  │
0x20019000 ├──────────────────────────────────┤
           │  Объекты IPC (Очереди, Mutex) │
           │  ├─ Очередь tx_request         │  50 KB
           │  ├─ Очередь sign_response      │
           │  ├─ Очередь display            │
           │  ├─ Очередь uart_log           │
           │  └─ Mutexes/семафоры           │
0x20026000 ├──────────────────────────────────┤
           │  Стеки задач (5 задач)         │  200 KB
           │  ├─ Стек задачи подписания     │  (32 KB каждый)
           │  ├─ Стек сетевой задачи       │
           │  ├─ Стек задачи дисплея       │
           │  ├─ Стек пользовательской      │
           │  └─ Стек задачи ввода-вывода  │
0x20046000 ├──────────────────────────────────┤
           │  Heap (динамическое выделение) │  250 KB
           │  ├─ Буферы LwIP               │
           │  ├─ malloc/free (крипто)       │
           │  └─ Буферы USB                │
0x200A2000 ├──────────────────────────────────┤
           │  Дескрипторы + данные RX LwIP │  96 KB
           │  ├─ Буфер кадров Ethernet     │
           │  ├─ Буферы UDP/TCP            │
           │  └─ Цепи mbuf                 │
0x200C0000 ├──────────────────────────────────┤
           │  [Оставшееся пространство]     │  192 KB
           │                                 │
0x20100000 └──────────────────────────────────┘
```

---

## 🔐 Цепь Доверия Безопасности

### С Загрузчиком stm32_secure_boot

```
┌─────────────────────────────────────────┐
│  Системная Flash STM32H743 @ 0x08000000 │
│                                          │
│  ┌──────────────────────────────────┐  │
│  │  ROM загрузчик ARM Cortex-M7     │  │
│  │  (1 KB, неизменяемый с завода)  │  │
│  │                                   │  │
│  │  Ответственность:                │  │
│  │  └─ Загрузить и выполнить       │  │
│  │     пользовательский загрузчик   │  │
│  │     @ 0x08000000                 │  │
│  └──────────────────────────────────┘  │
│            ↓ Переход к ↓                │
│  ┌──────────────────────────────────┐  │
│  │  Пользовательский загрузчик      │  │
│  │  (64 KB)                         │  │
│  │  (из stm32_secure_boot)          │  │
│  │                                   │  │
│  │  Ответственность:                │  │
│  │  ├─ Вычислить SHA-256 приложения│  │
│  │  ├─ Проверить подпись ECDSA    │  │
│  │  ├─ Переход на приложение если OK│ │
│  │  └─ Остановка + LED ошибка      │  │
│  └──────────────────────────────────┘  │
│            ↓ Проверенный переход ↓      │
│  ┌──────────────────────────────────┐  │
│  │  Приложение (1.5 MB)            │  │
│  │  (из CryptoWallet)              │  │
│  │                                   │  │
│  │  Ответственность:                │  │
│  │  ├─ Инициализировать оборудование│ │
│  │  ├─ Создать задачи FreeRTOS    │  │
│  │  ├─ Ждать ввода пользователя    │  │
│  │  ├─ Подписать транзакции        │  │
│  │  └─ Вернуть подписи              │  │
│  └──────────────────────────────────┘  │
└─────────────────────────────────────────┘

Цепь доверия:
ROM загрузчик
       ↓ доверяет
Пользовательский загрузчик (подписан ECDSA)
       ↓ доверяет
Приложение (подписано ECDSA)
       ↓ выполняет
Прошивка CryptoWallet
```

---

## 🎯 Сценарии Интеграции

### Сценарий 1: Автономный stm32_secure_boot

```
Вариант использования: Тестирование загрузчика + HID подписанта
├─ Сборка: make step2_hid
├─ Развёртывание: Один двоичный файл с загрузчиком + приложением
├─ Результат: Приложение, проверенное загрузчиком
└─ Преимущество: Проверенная цепь загрузки, но нет функций кошелька
```

### Сценарий 2: Только CryptoWallet (Без загрузчика)

```
Вариант использования: Кошелек без проверки загрузчика
├─ Сборка: make
├─ Развёртывание: Двоичный файл CryptoWallet @ 0x08010000
├─ Прошивка: Прямо на 0x08010000 (пропустить 0x08000000)
└─ Преимущество: Больше места для приложения, проще
   Недостаток: Нет проверки загрузки
```

### Сценарий 3: Загрузчик stm32_secure_boot + CryptoWallet (РЕКОМЕНДУЕТСЯ)

```
Вариант использования: Производственный кошелек с проверкой загрузки
├─ Сборка:
│  1. cd stm32_secure_boot && make bootloader
│  2. cd CryptoWallet && make
├─ Развёртывание:
│  1. Прошить bootloader.bin @ 0x08000000
│  2. Подписать приложение CryptoWallet (опционально)
│  3. Прошить cryptowallet.bin @ 0x08010000
├─ Результат: Проверенная цепь загрузки + полные функции кошелька
└─ Преимущество: Максимальная безопасность + функциональность
   Процесс: Более сложный, требует подписания
```

### Сценарий 4: Рабочий Процесс Разработки

```
Фаза 1: Разработка загрузчика
├─ cd stm32_secure_boot
├─ make bootloader
├─ make flash-bootloader
└─ Тестировать с step2_hid

Фаза 2: Разработка приложения
├─ cd CryptoWallet
├─ make
├─ make flash
└─ Тестировать подписание/сеть

Фаза 3: Интеграционное тестирование
├─ Загрузчик + приложение вместе
├─ Полная проверка цепи
├─ Закаливание безопасности
└─ Выпуск в производство
```

---

## 📈 Сравнение Метрик Кода

### stm32_secure_boot

```
├─ Всего файлов:          ~150 файлов
├─ Всего строк кода:      ~50,000 LOC
│  ├─ Загрузчик:          ~3,000 LOC
│  ├─ App (step2_hid):    ~8,000 LOC
│  ├─ FreeRTOS:          ~12,000 LOC
│  ├─ LwIP:              ~20,000 LOC
│  └─ Общее/примеры:      ~7,000 LOC
│
├─ Профили сборки:        12+
├─ Документация:          20+ файлов
└─ Статус:                Научный/Образовательный
```

### CryptoWallet

```
├─ Всего файлов:          ~200 файлов (включая документы)
├─ Всего строк кода:      ~60,000+ LOC
│  ├─ Ядро:               ~8,000 LOC
│  ├─ Задачи:             ~5,000 LOC
│  ├─ Крипто/кошелек:     ~3,000 LOC
│  ├─ Сеть/USB:           ~4,000 LOC
│  ├─ FreeRTOS:          ~12,000 LOC
│  ├─ LwIP:              ~20,000 LOC
│  └─ Документация:       ~8,000 LOC
│
├─ Варианты сборки:       3 (основной, минимальный, тест)
├─ Скрипты Python:        11+ скрипты тестирования/утилит
├─ Документация:          128+ файлы markdown (EN/RU/PL)
├─ Инфраструктура тестов: Полная (НОВОЕ) ✨
└─ Статус:                Готово к производству
```

---

## 🚀 Как Интегрировать stm32_secure_boot в CryptoWallet

### Способ 1: Использовать Существующие FreeRTOS/LwIP (Текущий)

CryptoWallet уже использует FreeRTOS и LwIP из stm32_secure_boot:

```bash
# Неявная зависимость
cd /data/projects/CryptoWallet

# FreeRTOS и LwIP - ссылки на версии stm32_secure_boot
# (если не включены прямо в CryptoWallet)
```

### Способ 2: Принять Загрузчик stm32_secure_boot

```bash
# Вариант A: Скопировать загрузчик в репо CryptoWallet
cp -r /data/projects/stm32_secure_boot/bootloader \
      /data/projects/CryptoWallet/

# Вариант B: Ссылка как подмодуль
cd /data/projects/CryptoWallet
git submodule add /data/projects/stm32_secure_boot bootloader

# Вариант C: Изменить Makefile CryptoWallet
# Добавить целевую сборку загрузчика
cat >> Makefile << 'EOF'
bootloader:
    $(MAKE) -C ../stm32_secure_boot bootloader
    cp ../stm32_secure_boot/build/bootloader.bin build/
EOF
```

### Способ 3: Использовать Тестирование RNG из CryptoWallet в stm32_secure_boot

```bash
# Скопировать инфраструктуру тестирования RNG
cp /data/projects/CryptoWallet/scripts/test_rng_*.py \
   /data/projects/stm32_secure_boot/scripts/

# Скопировать документацию
cp -r /data/projects/CryptoWallet/docs_src/crypto/rng* \
      /data/projects/stm32_secure_boot/docs/

# Теперь stm32_secure_boot может также тестировать RNG
cd /data/projects/stm32_secure_boot
make USE_RNG_DUMP=1
python3 scripts/test_rng_signing_comprehensive.py
```

---

## 🔧 Разрешение Зависимостей Сборки

### Проверка Зависимостей Makefile

```makefile
# Как CryptoWallet обеспечивает доступность FreeRTOS/LwIP

FREERTOS_PATH ?= ../stm32_secure_boot/FreeRTOS
LWIP_PATH ?= ../stm32_secure_boot

check-deps:
    @if [ ! -d "$(FREERTOS_PATH)" ]; then \
        echo "ОШИБКА: FreeRTOS не найден в $(FREERTOS_PATH)"; \
        exit 1; \
    fi
    @if [ ! -d "$(LWIP_PATH)" ]; then \
        echo "ОШИБКА: LwIP не найден в $(LWIP_PATH)"; \
        exit 1; \
    fi

all: check-deps compile link
```

---

## 📋 Контрольный Список Проверки Зависимостей

```bash
# Проверить доступность всех зависимостей

✅ STM32CubeH7
   └─ Расположение: /data/projects/STM32CubeH7/
   └─ Проверка: [ -d STM32CubeH7/Drivers ] && echo "OK"

✅ FreeRTOS
   └─ Расположение: /data/projects/stm32_secure_boot/FreeRTOS/
   └─ Проверка: [ -f FreeRTOS/Source/tasks.c ] && echo "OK"

✅ LwIP
   └─ Расположение: /data/projects/stm32_secure_boot/
   └─ Проверка: [ -d lwip ] && echo "OK"

✅ trezor-crypto
   └─ Расположение: /data/projects/CryptoWallet/ThirdParty/trezor-crypto/
   └─ Проверка: [ -f crypto.h ] && echo "OK"

✅ Цепь инструментов
   └─ Проверка: arm-none-eabi-gcc --version
   └─ Должно быть: >= 11.0

✅ Инструменты сборки
   └─ Проверка: which make && which st-flash
   └─ Требуется: GNU make, утилиты ST-Link
```

---

**Документ:** Зависимости и Отношения Проектов  
**Обновлено:** 2026-03-20  
**Статус:** Завершено
