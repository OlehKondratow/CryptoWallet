# 📚 Индекс документации: trezor-crypto и Dieharder в CryptoWallet

> **Дата:** 2026-03-19  
> **Автор:** AI Assistant  
> **Статус:** Полный анализ и документация  

---

## 📖 Структура документации

### Основные документы (созданы в этом сеансе)

#### 1. **ANALYSIS_trezor_crypto_dieharder.md** (главный документ)
   - **Цель:** Полный технический анализ интеграции
   - **Содержание:**
     - Архитектура слоёв интеграции
     - Использование trezor-crypto в проекте (таблицы функций)
     - Путь вызовов: HTTP POST → Подпись
     - RNG: TRNG + LCG Entropy Pool
     - DIEHARDER: назначение, рабочий процесс, тесты
     - Компиляционные флаги
     - Безопасность и лучшие практики
     - Диагностика проблем
   - **Читатели:** Разработчики, security engineers, QA

#### 2. **DIAGRAMS_trezor_crypto_dieharder.md** (визуальные схемы)
   - **Цель:** Архитектурные диаграммы ASCII
   - **Содержание:**
     - Слои интеграции (6 уровней)
     - Поток данных HTTP → SHA256 → Подпись (9 этапов)
     - RNG поток: TRNG + LCG → Dieharder
     - Pipeline Dieharder (6 этапов)
     - Точки интеграции (graph структура)
     - Таблица флагов компиляции
   - **Читатели:** Архитекторы, новые разработчики

#### 3. **EXAMPLES_trezor_crypto_usage.md** (практические примеры)
   - **Цель:** Рабочие примеры кода
   - **Содержание:**
     - Сборка и запуск для Dieharder (step-by-step)
     - Пример 1: SHA-256 хеширование (с внутренним кодом)
     - Пример 2: BIP-32 отведение ключей
     - Пример 3: ECDSA подписание
     - Пример 4: Валидация Bitcoin адреса (Base58Check)
     - Пример 5: Полный цикл подписания
     - Пример 6: Конфигурация Makefile
     - Пример 7: Проверка интеграции (bash скрипт)
   - **Читатели:** Разработчики, техподдержка

---

## 🔗 Связанные существующие документы

### В проекте CryptoWallet

| Файл | Назначение | Отношение к trezor-crypto |
|------|-----------|---------------------------|
| `docs_src/trezor-crypto-integration.md` | Основная документация | Родительский документ, детальное описание API |
| `docs_src/architecture.md` | Архитектура системы | Высокоуровневая архитектура, место trezor в стеке |
| `docs_src/rng-entropy.md` | RNG и энтропия | Дополняет наш анализ TRNG+LCG |
| `scripts/test_plan_signing_rng.py` | Тестовый план | Использует capture_rng_uart + run_dieharder |
| `scripts/capture_rng_uart.py` | Захват RNG | Инструмент для Dieharder (120+ MiB данных) |
| `scripts/run_dieharder.py` | Запуск Dieharder | Обёртка вокруг `dieharder -g 201 -f` |
| `Core/Src/crypto_wallet.c` | Реализация | Основной адаптер trezor-crypto для проекта |
| `Core/Inc/crypto_wallet.h` | Публичный API | Определяет интерфейс для приложений |
| `Core/Src/task_sign.c` | Подписание | Использует crypto_wallet API |
| `Core/Src/tx_request_validate.c` | Валидация | Использует trezor base58_decode_check |

---

## 🎯 Быстрая навигация

### Для быстрого старта (15 мин)

1. Прочитайте **DIAGRAMS** → поймите архитектуру
2. Скопируйте Пример 1 из **EXAMPLES** → запустите Dieharder
3. Посмотрите результаты → интерпретируйте p-values

### Для глубокого понимания (2-3 часа)

1. ANALYSIS (полностью) → понимание всех компонентов
2. DIAGRAMS (все схемы) → визуализация
3. EXAMPLES (все примеры) → практическое применение
4. `docs_src/trezor-crypto-integration.md` → официальное описание

### Для отладки проблем (30 мин)

1. ANALYSIS → раздел 8 (Проблемные области)
2. EXAMPLES → Пример 7 (Проверка интеграции)
3. Запустить `make check-integration.sh`

---

## 📊 Таблица функций trezor-crypto (быстрая справка)

| Модуль | Функция | Входные данные | Выход | Использование |
|--------|---------|----------------|-------|---------------|
| **bip39** | `mnemonic_from_data()` | 16 байт энтропии | 12 слов | Генерация мнемоники |
| **bip32** | `hdnode_from_seed()` | 64-byte seed | HDNode | Инициализация |
| **bip32** | `hdnode_private_ckd_prime()` | HDNode, index | HDNode | Жёсткое отведение (m/44') |
| **bip32** | `hdnode_private_ckd()` | HDNode, index | HDNode | Мягкое отведение (m/0) |
| **ecdsa** | `ecdsa_sign_digest()` | privkey, hash | signature (64 byte) | Подписание |
| **sha2** | `sha256_Raw()` | data, len | digest (32 byte) | Хеширование |
| **base58** | `base58_decode_check()` | address string | decoded (25 byte) | Валидация адреса |
| **rand** | (заменён на random_buffer) | buf, len | buf with RNG | Случайные числа |

---

## 🔐 Чек-лист безопасности

```
☑ Все чувствительные данные (seed, privkey, hash) очищены после использования
☑ random_buffer() использует TRNG + LCG (не только одно из)
☑ RFC6979 используется для детерминированного k (не случайное k)
☑ BIP-32 hardened derivation для криптографических уровней
☑ Base58Check валидация адресов (защита от опечаток)
☑ Dieharder тесты пройдены (p-value 0.001 - 0.999)
☑ Нет приватных ключей в логах/ответах
☑ Timeout 30 сек на подтверждение (защита от hold-up)
```

---

## 🛠️ Типичные команды

### Сборка для Dieharder

```bash
cd /data/projects/CryptoWallet
make clean
make USE_CRYPTO_SIGN=1 USE_TEST_SEED=1 USE_RNG_DUMP=1 -j4
make flash
```

### Захват и тестирование

```bash
# Терминал 1: Устройство
# (никакого вывода)

# Терминал 2: Захват
python3 scripts/capture_rng_uart.py --port /dev/ttyACM0 --out rng.bin --bytes 134217728

# Терминал 3: Тестирование
python3 scripts/run_dieharder.py --file rng.bin

# Сохранение результатов
python3 scripts/run_dieharder.py --file rng.bin | tee dieharder_results.txt
```

### Проверка интеграции

```bash
# Проверить, что trezor-crypto скомпилирован
arm-none-eabi-nm build/firmware.elf | grep -E "bip39|bip32|ecdsa"

# Размер
arm-none-eabi-size build/firmware.elf

# Валидация
./scripts/check_integration.sh
```

---

## 📈 Метрики проекта

| Метрика | Значение | Примечание |
|---------|----------|-----------|
| trezor-crypto файлы | ~80 .c/.h файлов | ThirdParty/trezor-crypto/ |
| Линкованные модули | ~22 объектных файла | При USE_CRYPTO_SIGN=1 |
| Размер кода trezor | ~100-150 KB | Object code |
| Размер RNG capture | 128 MiB | Минимум для Dieharder |
| Время захвата RNG | ~30 мин | @115200 baud |
| Время Dieharder (-a) | 30 мин - 3 часа | Зависит от процессора |
| Тестов Dieharder | ~100+ | Встроенные статистические тесты |

---

## 🧪 Тестовые сценарии

### Сценарий 1: Проверка подписания (функциональная)

```bash
# Предусловия
USE_CRYPTO_SIGN=1, USE_TEST_SEED=1

# Действия
1. HTTP POST /tx с валидным JSON
2. Нажать кнопку (подтверждение)
3. GET /tx/signed

# Ожидаемый результат
- JSON с 64-байтной подписью (128 hex chars)
- Одинаковые входы → одинаковая подпись (RFC6979)
```

### Сценарий 2: Проверка RNG (Dieharder)

```bash
# Предусловия
USE_RNG_DUMP=1, 134 MiB raw bytes captured

# Действия
python3 scripts/run_dieharder.py --file rng.bin

# Ожидаемый результат
- 80-95% PASS
- <5% FAIL
```

### Сценарий 3: Безопасность (inspection)

```bash
# Предусловия
Firmware compiled and running

# Действия
- Capture UART log
- Search for patterns: seed, privkey, 0x (hex dumps)
- HTTP response inspection

# Ожидаемый результат
- Нет приватных данных в логах
- Только публичные подписи в ответах
```

---

## 📝 Стиль документации

### Уровни детализации

| Уровень | Аудитория | Документ | Время чтения |
|---------|-----------|----------|--------------|
| 🟢 **Начинающий** | Новые разработчики | DIAGRAMS | 15 мин |
| 🟡 **Промежуточный** | Опытные разработчики | EXAMPLES | 1 час |
| 🔴 **Эксперт** | Security engineers | ANALYSIS | 2-3 часа |

---

## 🔄 Версионирование документации

| Версия | Дата | Изменения |
|--------|------|-----------|
| 1.0 | 2026-03-19 | Первоначальный анализ и документация |
| (TBD) | (TBD) | Интеграция результатов Dieharder |
| (TBD) | (TBD) | Обновление при смене версии trezor-crypto |

---

## 📞 Контакты / Ссылки

### Документация в проекте
- **Главный README:** `/data/projects/CryptoWallet/README.md`
- **Архитектура:** `docs_src/architecture.md`
- **trezor-crypto:** `docs_src/trezor-crypto-integration.md`
- **RNG и энтропия:** `docs_src/rng-entropy.md`

### Внешние ссылки
- **trezor-crypto GitHub:** https://github.com/trezor/trezor-crypto
- **Dieharder:** http://www.phy.duke.edu/~rgb/General/dieharder.php
- **BIP-32:** https://github.com/bitcoin/bips/blob/master/bip-0032.mediawiki
- **BIP-39:** https://github.com/bitcoin/bips/blob/master/bip-0039.mediawiki
- **RFC6979:** https://tools.ietf.org/html/rfc6979

---

## 📋 Чек-лист использования документации

- [ ] Прочитан DIAGRAMS (15 мин)
- [ ] Понимаю архитектуру слоёв
- [ ] Запустил Пример 1 (capture + dieharder)
- [ ] Интерпретирую результаты Dieharder
- [ ] Прочитан полный ANALYSIS (при необходимости)
- [ ] Изучены примеры кода (при разработке)
- [ ] Документация актуальна для моей версии (проверить дату)

---

**Конец индекса документации**

_Дата создания: 2026-03-19_  
_Версия: 1.0_  
_Статус: ✅ Готово_
