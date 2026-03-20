# Настройка Среды Тестирования

## ✅ Выполненные Работы

Все зависимости Python установлены и настроены для пакета тестирования CryptoWallet.

### Созданные Файлы

| Файл | Размер | Назначение |
|------|--------|-----------|
| `requirements-test.txt` | 696 B | Спецификации пакетов Python с диапазонами версий |
| `requirements-test-lock.txt` | 705 B | Точные версии всех установленных пакетов |
| `install-test-deps.sh` | 3.4 K | Автоматический установщик системных и Python зависимостей |
| `INSTALL_DIEHARDER.txt` | 2.6 K | Быстрый справочник по установке dieharder |
| `docs_src/INSTALL_TEST_DEPS.md` | ~7 K | Полное руководство по установке с устранением проблем |

### Установленные Пакеты Python (22 всего)

#### Основные Зависимости (Обязательные)
- **pyserial 3.5** - Коммуникация с последовательным портом устройства CryptoWallet
- **requests 2.32.5** - HTTP-клиент для коммуникации REST API
- **colorama 0.4.6** - Цветной вывод терминала

#### Инструменты Разработки (Опционально но рекомендуется)
- **pytest 7.4.4** - Фреймворк модульного тестирования
- **black 24.10.0** - Форматер кода
- **flake8 7.3.0** - Линтер кода

#### Управление Сборкой и Пакетами
- **setuptools 82.0.1**, **pip 26.0.1**, **wheel 0.46.3**

#### Транзитивные Зависимости
- charset-normalizer, idna, urllib3 (поддержка HTTP)
- click, pathspec, platformdirs (инструменты CLI)
- И другие для линтинга/форматирования

## 🐛 Решённая Проблема

**Проблема:** `pytest-serial>=0.0.9` - Этот пакет не существует в PyPI

**Основная причина:** Неправильная спецификация пакета в исходных требованиях

**Решение:** Удален несуществующий пакет, сохранены только действительные зависимости

## 🚀 Быстрый Старт

### 1. Установите Системные Зависимости

**Автоматически (Рекомендуется):**
```bash
cd /data/projects/CryptoWallet
./install-test-deps.sh
```

**Ручно (Выберите Вашу ОС):**

Linux (Debian/Ubuntu):
```bash
sudo apt update && sudo apt install -y dieharder
```

Linux (Fedora/RHEL):
```bash
sudo dnf install -y dieharder
```

macOS:
```bash
brew install dieharder
```

### 2. Активируйте Виртуальное Окружение

```bash
source .venv-test/bin/activate
```

### 3. Проверьте Установку

```bash
# Проверьте dieharder
dieharder --version

# Проверьте пакеты Python
python3 -c "import serial, requests, colorama, pytest; print('✓ All packages OK')"

# Тестируйте скрипт теста
python3 scripts/test_rng_signing_comprehensive.py --help
```

## 📖 Документация

- **Детали установки**: `docs_src/INSTALL_TEST_DEPS.md`
- **Настройка виртуального окружения**: `docs_src/VENV_SETUP.md`
- **Руководство тестирования**: `docs_src/TESTING_GUIDE_RNG_SIGNING.md`
- **Быстрый старт**: `VENV_QUICKSTART.txt`
- **Требования**: `requirements-test.txt` и `requirements-test-lock.txt`

## 🎯 Доступные Режимы Тестирования

После настройки всего вы можете запускать:

```bash
# Тест RNG (захват и анализ энтропии)
python3 scripts/test_rng_signing_comprehensive.py --mode rng

# Анализ Dieharder (статистические тесты)
python3 scripts/test_rng_signing_comprehensive.py --mode dieharder

# Тест подписания транзакций
python3 scripts/test_rng_signing_comprehensive.py --mode signing --ip 192.168.0.10

# Полный пакет тестов
python3 scripts/test_rng_signing_comprehensive.py --mode verify-all
```

## ✅ Контрольный Список Проверки

- [ ] `dieharder --version` показывает номер версии
- [ ] `.venv-test/bin/python3 --version` показывает Python 3.12
- [ ] `python3 -c "import serial"` успешно (без ImportError)
- [ ] `python3 -c "import requests"` успешно
- [ ] `python3 scripts/test_rng_signing_comprehensive.py --help` показывает полную справку
- [ ] Устройство USB/UART подключено (для `--mode rng`)

## 📝 Файлы Требований

### requirements-test.txt
Содержит гибкие ограничения версий для разработки:
- Используйте когда: Установка в новую среду, гибкие версии приемлемы
- Установите с: `pip install -r requirements-test.txt`

### requirements-test-lock.txt
Содержит точно закрепленные версии для воспроизводимости:
- Используйте когда: Необходима точная воспроизводимость, развертывание в производство
- Установите с: `pip install -r requirements-test-lock.txt`
- Сгенерируйте: `pip freeze > requirements-test-lock.txt`

## 🐛 Устранение Неполадок

### "dieharder: command not found"
```bash
# Проверьте установлен ли
which dieharder

# Установите для вашей ОС (см. раздел Быстрый Старт)
```

### "ModuleNotFoundError: No module named 'serial'"
```bash
# Активируйте venv
source .venv-test/bin/activate

# Переустановите пакеты
pip install -r requirements-test.txt

# Проверьте
python3 -c "import serial; print('OK')"
```

### "Permission denied" на /dev/ttyUSB0
```bash
# Добавьте пользователя в группу dialout (Linux)
sudo usermod -a -G dialout $USER
newgrp dialout

# Проверьте
ls -l /dev/ttyUSB0
```

### По-прежнему есть проблемы?
См. `docs_src/INSTALL_TEST_DEPS.md` для полного руководства по устранению проблем.

## 📊 Информация об Окружении

- **Версия Python**: 3.12.7
- **Виртуальное Окружение**: `.venv-test/`
- **Пакеты Всего**: 22
- **Дата Настройки**: 2026-03-20
- **Статус**: ✅ Готово

## 🔄 Обновление Зависимостей

Для обновления пакетов при сохранении ограничений версии:

```bash
source .venv-test/bin/activate
pip install --upgrade -r requirements-test.txt
pip freeze > requirements-test-lock.txt
```

## 📚 Связанные Файлы

```
.
├── .venv-test/                          # Виртуальное окружение
├── requirements-test.txt                # Спецификации пакетов
├── requirements-test-lock.txt           # Закрепленные версии
├── install-test-deps.sh                 # Автоустановщик
├── INSTALL_DIEHARDER.txt               # Справочник Dieharder
├── activate-tests.sh                    # Помощник активации venv
├── run-tests.sh                         # Скрипт запуска тестов
├── VENV_QUICKSTART.txt                 # Быстрый старт
├── docs_src/
│   ├── INSTALL_TEST_DEPS.md            # Полное руководство конфигурации
│   ├── VENV_SETUP.md                   # Детали venv
│   └── TESTING_GUIDE_RNG_SIGNING.md    # Руководство тестирования
└── scripts/
    └── test_rng_signing_comprehensive.py  # Основной тестовый скрипт
```

---

**Последнее Обновление**: 2026-03-20  
**Статус**: ✅ Полный и Проверенный
