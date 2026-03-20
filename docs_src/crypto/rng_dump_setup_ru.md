# Включение RNG Dump в Прошивке CryptoWallet

## Быстрое Решение

Тест захвата RNG не работает, потому что **прошивка не настроена на вывод сырых данных RNG на UART**.

Вам нужно **пересобрать прошивку с флагом `USE_RNG_DUMP=1`**.

## Пошаговое Руководство

### 1. Проверьте Текущий Makefile

```bash
cd /data/projects/CryptoWallet
grep "USE_RNG_DUMP" Makefile
```

**Ожидаемо**: Пустой вывод (в настоящее время не установлено)

### 2. Добавьте RNG_DUMP в Makefile

Добавьте эту строку после определения `USE_TEST_SEED` (около строки 59):

```makefile
# USE_RNG_DUMP=1: вывод сырых данных RNG на UART (для тестирования Dieharder)
# ПРЕДУПРЕЖДЕНИЕ: отключает обычный вывод UART - отправляются только двоичные данные RNG
USE_RNG_DUMP ?= 0
```

#### Расположение в Makefile:

Найдите этот раздел (строки ~57-67):

```makefile
# USE_TEST_SEED=1: известная тестовая мнемоника в wallet_seed.c; принудительно USE_CRYPTO_SIGN
# Подразумевает USE_CRYPTO_SIGN=1
USE_TEST_SEED ?= 0
ifeq ($(USE_TEST_SEED),1)
USE_CRYPTO_SIGN := 1
```

Добавьте вашу строку после этого, перед блоком USE_CRYPTO_SIGN.

### 3. Добавьте Флаг в CFLAGS

Найдите раздел CFLAGS где USE_TEST_SEED добавляется (около строки 60-70) и добавьте:

```makefile
ifeq ($(USE_RNG_DUMP),1)
CFLAGS += -DUSE_RNG_DUMP
endif
```

Это должно быть **после** блока `USE_TEST_SEED` и **перед** блоком `USE_CRYPTO_SIGN`.

### 4. Пересоберите Прошивку

```bash
cd /data/projects/CryptoWallet

# Вариант A: Полная очистка и пересборка с флагом
make clean
make USE_RNG_DUMP=1 -j$(nproc)

# Вариант B: Если вы хотите только RNG dump (без криптографического подписания)
make clean
make USE_RNG_DUMP=1 -j$(nproc)

# Вариант C: С криптографией и RNG dump
make clean
make USE_CRYPTO_SIGN=1 USE_RNG_DUMP=1 -j$(nproc)
```

### 5. Загрузите на Устройство

```bash
# Это зависит от вашей конфигурации. Типичные варианты:

# Вариант A: STM32 ST-Link
make flash

# Вариант B: DFU (если используется загрузчик DFU)
make dfu

# Вариант C: Ручная загрузка с использованием st-flash
st-flash write build/*.elf 0x08000000

# Вариант D: Использование OpenOCD
openocd -f interface/stlink-v2-1.cfg -f target/stm32h7x.cfg \
  -c "program build/*.elf verify reset exit"
```

**Примечание**: Проверьте целевой флеш-объект в вашем проекте:
```bash
grep -n "^flash:" Makefile
```

### 6. Проверьте Загруженную Прошивку

После загрузки устройство должно выводить **только двоичные данные** на UART (без текста):

```bash
# Быстрый тест (остановка Ctrl+C)
python3 -m serial.tools.miniterm /dev/ttyACM0 115200 --raw

# Вы должны увидеть двоичный мусор - сырые данные RNG
# НЕ: текстовый вывод, приглашения меню или ничего
```

## Интеграция Кода Прошивки

Код прошивки должен уже иметь функциональность RNG dump. Проверьте:

```bash
# Поиск USE_RNG_DUMP в исходном коде
grep -r "USE_RNG_DUMP" /data/projects/CryptoWallet/Core/

# Должно найти конфигурацию UART/задач, которая:
# - Выводит сырые байты RNG при включении
# - Обходит обычные сообщения UART
```

Если не найдено, может потребоваться добавить в `Core/Src/main.c` или задачу UART.

## Типичные Проблемы

### Проблема 1: "make: target flash not found"
Ваш проект может использовать другой метод загрузки. Проверьте:
```bash
make help | grep -i flash
```

### Проблема 2: "make: command not found"
Вам нужно установить ARM embedded toolchain:
```bash
sudo apt install build-essential arm-none-eabi-gcc
```

### Проблема 3: "No such file or directory: stm32cubeh7"
Зависимости не найдены. Проверьте:
```bash
ls -d ../STM32CubeH7 ../stm32-ssd1306 ../stm32_secure_boot
```

Если отсутствуют, склонируйте их:
```bash
cd /data/projects
git clone https://github.com/STMicroelectronics/STM32CubeH7
# и т.д.
```

## Тестирование После Сборки

После загрузки прошивки протестируйте RNG dump:

```bash
# Тест 1: Сырое чтение
dd if=/dev/ttyACM0 bs=1 count=100 2>/dev/null | xxd

# Тест 2: Запустите тестовый скрипт
source .venv-test/bin/activate
python3 scripts/test_rng_signing_comprehensive.py --mode rng
```

Тест теперь должен:
1. Открыть последовательный порт ✓
2. Обнаружить доступные данные ✓
3. **Успешно прочитать двоичные данные** ✓

## Ожидаемый Вывод Сборки

```
Compiling Core/Src/main.c ...
Compiling Core/Src/task_rng.c ...
Linking ...
Creating binary ...
arm-none-eabi-objcopy -O binary build/firmware.elf build/firmware.bin
Firmware ready: build/firmware.elf (128 KB)
```

## Проверка

После загрузки проверьте флаги сборки прошивки:

```bash
# Проверьте содержит ли двоичный файл флаг в строках
arm-none-eabi-strings build/firmware.elf | grep -i rng

# Или проверьте временную метку сборки
ls -l build/firmware.elf
# Должно показать последнее время
```

## Откат (Возврат к Обычной Прошивке)

Для возврата к обычной прошивке без RNG dump:

```bash
cd /data/projects/CryptoWallet
make clean
make -j$(nproc)  # Без флагов - использует значения по умолчанию
make flash
```

Это будет пересобрано с:
- `USE_RNG_DUMP=0` (обычный вывод UART)
- `USE_LWIP=1` (Сеть LwIP, по умолчанию)
- `USE_CRYPTO_SIGN=0` (без подписания, если вы его не установите)

## Следующие Шаги

1. Отредактируйте Makefile и добавьте поддержку USE_RNG_DUMP
2. Пересоберите: `make clean && make USE_RNG_DUMP=1 -j$(nproc)`
3. Загрузите на устройство: `make flash`
4. Проверьте: `python3 -m serial.tools.miniterm /dev/ttyACM0 115200 --raw`
5. Тестируйте: `python3 scripts/test_rng_signing_comprehensive.py --mode rng`

---

**Смотрите также:**
- `TROUBLESHOOT_RNG_CAPTURE.md` - Полное руководство по устранению неполадок
- `docs_src/testing/rng_capture_testing.md` - Полное руководство тестирования
- Документация прошивки в `docs_src/crypto/`
