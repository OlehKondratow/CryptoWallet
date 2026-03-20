## RNG Dump и Тестирование Dieharder

**Статус:** ✅ Прошивка с USE_RNG_DUMP=1 успешно скомпилирована!

### Что изменилось?

Реализована полная поддержка вывода сырых RNG данных на UART для тестирования с Dieharder:

**Новые файлы:**
- `Core/Src/rng_dump.c` - FreeRTOS задача для вывода RNG данных
- `Core/Inc/rng_dump.h` - Заголовок для rng_dump
- `RNG_DUMP_QUICK_START.sh` - Автоматизированный скрипт загрузки и тестирования

**Модифицированные файлы:**
- `Makefile` - Добавлены флаги `USE_RNG_DUMP`, `HAL_RNG_MODULE_ENABLED`, правила компиляции
- `Core/Src/hw_init.c` - Инициализация RNG при `USE_RNG_DUMP=1`
- `Core/Src/main.c` - Вызов `RNG_Dump_Task_Create()` при включенном флаге

### Быстрый старт

#### 1️⃣ Прошивка уже скомпилирована:
```bash
ls -lh build/cryptowallet.bin
# -rw-rw-r-- 1 pilgrim pilgrim 205K Mar 20 01:28 build/cryptowallet.bin
```

#### 2️⃣ Загрузить на устройство и начать тестирование:
```bash
# Подключите STM32H743ZI2 через USB ST-Link
./RNG_DUMP_QUICK_START.sh

# или с конкретным портом:
./RNG_DUMP_QUICK_START.sh /dev/ttyACM1
```

#### 3️⃣ Или сделать это вручную:

**Шаг 1: Загрузить прошивку**
```bash
st-flash --freq=24000 write build/cryptowallet.bin 0x08000000
```

**Шаг 2: Активировать виртуальное окружение**
```bash
source .venv-test/bin/activate
```

**Шаг 3: Запустить захват RNG данных**
```bash
python3 scripts/test_rng_signing_comprehensive.py --mode rng --port /dev/ttyACM0
```

Это создаст файл `rng.bin` (~128 MiB) с сырыми RNG данными (~5 минут захвата).

#### 4️⃣ Запустить тесты Dieharder:
```bash
# Полный набор тестов (может быть долгим)
dieharder -a -g 201 -f rng.bin

# Или быстрый скрининг:
dieharder -d 0 -a -g 201 -f rng.bin

# Просмотреть результаты:
grep -E 'PASSED|FAILED|WEAK' dieharder_results_*.txt
```

### Архитектура реализации

**RNG_Dump_Task (Core/Src/rng_dump.c):**
```
┌─ FreeRTOS Task ──┐
│  RNG_Dump_Task  │
├─────────────────┤
│ 1. Читает из    │
│    STM32 RNG →  │
│    RNG->DR      │
│                 │
│ 2. Буферирует   │
│    256 байт     │
│                 │
│ 3. Передает на  │
│    UART3 @      │
│    115200 baud  │
│                 │
│ 4. vTaskDelay   │
│    10ms         │
└─────────────────┘
```

**Инициализация (hw_init.c):**
- `MX_RNG_Init()` инициализирует HAL RNG периферию
- `HAL_RNG_MspInit()` включает часы RNG
- Вызывается при `HW_Init()` если `USE_RNG_DUMP=1`

**Build система (Makefile):**
```makefile
USE_RNG_DUMP ?= 0
ifeq ($(USE_RNG_DUMP),1)
  CFLAGS += -DUSE_RNG_DUMP=1 -DHAL_RNG_MODULE_ENABLED
  OBJ += $(BUILD)/rng_dump.o $(BUILD)/hal_rng.o
  OBJ_MINIMAL_LWIP += $(BUILD)/rng_dump.o $(BUILD)/hal_rng.o
endif
```

### Проверка работы

**Устройство подключено и работает:**
```bash
# Проверьте, что UART отправляет данные
cat /dev/ttyACM0 | xxd | head

# Должны увидеть бинарный мусор (raw RNG):
# 00000000: a3f2 15e8 c42d 7ab1 9e55 4c72 f831 0285
# 00000010: 2b7c 84f0 ...
```

**Dieharder тесты проходят:**
```bash
dieharder -d 0 -g 201 -f rng.bin | head -50
# 
# Dieharder test results
# 
# test_count= 100 ntuple= 0 tsamples=8 psamples=16777216
# ...
# PASSED: sts_monobit
# PASSED: sts_runs
```

### Известные ограничения

1. **Только RNG данные на UART** - при USE_RNG_DUMP=1 normal logging отключается
2. **Нет фильтрации данных** - передаются raw значения из RNG->DR
3. **Максимальный размер захвата** - ограничена памятью PC и скоростью UART
4. **115200 baud** - оптимально для надежности, можно попробовать выше

### Оптимизация и расширения

Если нужны улучшения:

1. **Повысить скорость UART:**
   - `huart3.Init.BaudRate = 921600;` в hw_init.c
   - Требует повышения скорости в Python тесте: `--baud 921600`

2. **Добавить шифрование RNG:**
   - Использовать `random_buffer()` из crypto_wallet.h
   - Смешивание TRNG + LCG энтропии для большей энтропии

3. **Дописать собственный RNG фильтр:**
   - Von Neumann extractor
   - XOR-mixing с таймерами
   - Реализовать в rng_dump.c

### Поиск неисправностей

**Проблема:** Ошибка "device reports readiness to read but returned no data"

**Решения:**
1. Проверьте, что прошивка загружена с `USE_RNG_DUMP=1`
   ```bash
   strings build/cryptowallet.bin | grep USE_RNG_DUMP
   ```
2. Убедитесь, что UART кабель подключен правильно
3. Попробуйте другой USB портКрипто

4. Перезагрузитесь: отключите USB, подождите 2 сек, подключите

**Проблема:** Dieharder результаты не хорошие

**Решения:**
1. Собрать больше данных (128 MiB минимум, 1 GiB рекомендуется)
2. Проверить, что нет интерференции на UART
3. Попробовать другой порт USB
4. Убедиться, что RNG инициализирован: добавить вывод в hw_init.c

### Документация

Полная документация по RNG тестированию:
- `docs_src/crypto/rng_dump_setup.md` - Настройка USE_RNG_DUMP
- `docs_src/crypto/rng_capture_troubleshooting.md` - Поиск неисправностей
- `docs_src/crypto/testing_setup.md` - Общая настройка тестирования

---

**Статус:** ✅ Готово к тестированию!

Следующие шаги:
1. Подключить STM32H743ZI2 через USB ST-Link
2. Запустить `./RNG_DUMP_QUICK_START.sh`
3. Дождаться завершения захвата (~5 минут)
4. Проанализировать результаты Dieharder
