# 📚 Анализ системы автогенерации документации в CryptoWallet

**Дата анализа:** 2026-03-19  
**Статус:** ✅ Система полностью настроена

---

## 📋 Краткое резюме

В проекте CryptoWallet реализована **комплексная система автогенерации документации** с использованием:

- **Doxygen** — для парсинга исходного кода и генерации HTML/XML
- **MkDocs Material** — для генерации веб-сайта документации
- **Python скрипты** — для обработки и конвертирования документации
- **Make targets** — для простого запуска процесса

---

## 🔄 Архитектура системы

```
┌─────────────────────────────────────────────────────┐
│         ИСХОДНЫЕ ДАННЫЕ                             │
├─────────────────────────────────────────────────────┤
│ • Core/Src/*.c, Core/Inc/*.h (Doxygen комментарии)│
│ • docs_src/*.md (ручная документация)              │
│ • scripts/*.py (Python скрипты)                    │
│ • Doxyfile (конфигурация Doxygen)                  │
└─────────────────────────────────────────────────────┘
                        ↓
        ┌───────────────┬───────────────┐
        ↓               ↓               ↓
┌──────────────┐  ┌──────────────┐  ┌──────────────┐
│  Doxygen     │  │  Python      │  │  MkDocs      │
│  (код C)     │  │  (скрипты)   │  │  (сайт)      │
└──────────────┘  └──────────────┘  └──────────────┘
        ↓               ↓               ↓
┌──────────────┐  ┌──────────────┐  ┌──────────────┐
│ HTML + XML   │  │ Markdown MD  │  │ docs/        │
│ docs_doxygen/│  │ docs_src/    │  │ index.html   │
└──────────────┘  └──────────────┘  └──────────────┘
```

---

## 🛠️ Make Targets для документации

### 1. **make docs** — Полное построение
```bash
make docs
```

**Что делает:**
1. Проверяет наличие MkDocs
2. Регенерирует `docs_src/testing-plan-signing-rng.md`
3. Регенерирует `docs_src/reference-code.md` (из заголовков)
4. Собирает HTML сайт в `docs/index.html`

**Требует:**
- MkDocs Material (`requirements-docs.txt`)
- Python 3
- Исходные файлы в `docs_src/`

---

### 2. **make docs-md** — Только Markdown
```bash
make docs-md
```

**Что делает:**
1. Регенерирует `docs_src/testing-plan-signing-rng.md`
2. Регенерирует `docs_src/reference-code.md`
3. **НЕ** запускает MkDocs

**Результат:**
- Только `.md` файлы, без HTML

---

### 3. **make docs-code-md** — Только reference-code.md
```bash
make docs-code-md
```

**Что делает:**
- Парсит заголовки из `Core/Src/*.c`, `Core/Inc/*.h`
- Генерирует `docs_src/reference-code.md`

**Использует:** `scripts/generate_code_reference_md.py`

---

### 4. **make docs-doxygen** — Doxygen + Update README
```bash
make docs-doxygen
```

**Что делает:**
1. Запускает `doxygen Doxyfile` → генерирует XML/HTML
2. Обновляет README "Project Structure" таблицу
3. Генерирует `docs_doxygen/md/*.md` (опционально)
4. Запускает `update_docs_src_index.py`

**Результат:**
- `docs_doxygen/html/` — HTML документация
- `docs_doxygen/xml/` — XML для скриптов
- `docs_doxygen/md/` — Markdown для каждого файла
- Обновлены README разделы

---

### 5. **make docs-serve** — Live server
```bash
make docs-serve
```

**Что делает:**
- Запускает локальный сервер MkDocs с live reload
- Доступно: `http://localhost:8000`

---

## 📝 Python скрипты для генерации

### 1. **scripts/generate_code_reference_md.py**

**Назначение:** Генерировать `docs_src/reference-code.md` из Doxygen комментариев

**Парсит:**
- `@file`, `@brief`, `@details` из `.c` файлов
- Docstrings из `.py` файлов

**Выход:**
- Одноименный Markdown файл с описанием каждого модуля

**Использование:**
```bash
python3 scripts/generate_code_reference_md.py -o docs_src/reference-code.md
```

**Конфигурация:**
- Ищет `@brief` и `@details` в комментариях
- Разрешены теги: `file`, `brief`, `details`, `note`, `warning`, `attention`, `copyright`

---

### 2. **scripts/update_readme.py**

**Назначение:** Обновлять README.md таблицу "Project Structure"

**Парсит:**
- `docs_doxygen/xml/` (Doxygen XML output)
- Извлекает `@brief` и `@details` для каждого файла

**Что обновляет:**
1. Таблица в README "## Project Structure"
2. Опционально: генерирует `docs_doxygen/md/<файл>.md`

**Использование:**
```bash
doxygen Doxyfile
python3 scripts/update_readme.py
python3 scripts/update_readme.py --md-dir docs_doxygen/md
```

**Параметры:**
```
--readme PATH       Путь к README (default: README.md)
--xml PATH         Путь к Doxygen XML (default: docs_doxygen/xml)
--md-dir PATH      Генерировать .md в директорию
```

---

### 3. **scripts/update_docs_src_index.py**

**Назначение:** Генерировать индекс документации в README

**Парсит:**
- Все `.md` файлы в `docs_src/`
- Ищет заголовок `## Краткий обзор` или `## Summary`
- Извлекает первый параграф

**Генерирует:**
- Таблицу в README между маркерами:
  ```html
  <!-- DOXYGEN_DOCS_SRC_INDEX -->
  <!-- /DOXYGEN_DOCS_SRC_INDEX -->
  ```

**Использование:**
```bash
python3 scripts/update_docs_src_index.py
python3 scripts/update_docs_src_index.py --dry-run  # Только показать, не писать
```

**Параметры:**
```
--docs-dir PATH       Путь к docs_src (default: docs_src)
--readme PATH         Путь к README (default: README.md)
--section-title STR   Заголовок секции
--dry-run             Показать, но не писать
```

---

### 4. **scripts/test_plan_signing_rng.py**

**Назначение:** Генерировать тестовый план для подписания и RNG

**Генерирует:**
- `docs_src/testing-plan-signing-rng.md`
- Содержит DIEHARDER чек-лист и тестовые сценарии

**Использование:**
```bash
python3 scripts/test_plan_signing_rng.py
python3 scripts/test_plan_signing_rng.py --write docs_src/testing-plan-signing-rng.md
```

---

## 📂 Структура документации

### Input

```
docs_src/
├── *.md                  # Ручная документация
├── README.md            # Индекс
├── crypto/              # Крипто-специфичная документация
│   ├── README.md
│   ├── ANALYSIS_*.md
│   ├── EXAMPLES_*.md
│   └── ... (30 файлов)
│
Src/, Core/
├── *.c, *.h             # Исходный код с Doxygen комментариями
```

### Output

```
docs_doxygen/
├── html/                # HTML документация (Doxygen)
├── xml/                 # XML для обработки скриптами
└── md/                  # Markdown для каждого файла

docs/
├── index.html           # Главная страница (MkDocs)
├── ...                  # Вся документация в HTML
```

---

## 🔧 Конфигурация

### Doxyfile

**Главные параметры:**

```makefile
PROJECT_NAME           = "CryptoWallet"
OUTPUT_DIRECTORY       = docs_doxygen

# Input
INPUT                  = Core/Src Core/Inc Src Drivers/ssd1306 docs_src
FILE_PATTERNS          = *.c *.h *.md
EXCLUDE                = ThirdParty/ build/

# Output formats
GENERATE_HTML          = YES
GENERATE_XML           = YES
GENERATE_LATEX         = NO

# Optimization for C
OPTIMIZE_OUTPUT_FOR_C  = YES
MARKDOWN_SUPPORT       = YES
AUTOLINK_SUPPORT       = YES
```

---

## 📊 Workflow: От кода к сайту

```
1. РАЗРАБОТКА
   ├─ Пишу код в Core/Src/*.c
   ├─ Добавляю Doxygen комментарии:
   │  /**
   │   * @file my_module.c
   │   * @brief Краткое описание
   │   * @details Подробное описание
   │   */
   └─ Коммичу в git

2. ЛОКАЛЬНАЯ ГЕНЕРАЦИЯ
   ├─ make docs-md      # Быстро: генерирую .md
   ├─ make docs-code-md # Генерирую reference-code.md
   ├─ make docs-doxygen # Полностью: Doxygen + XML + обновляю README
   └─ make docs         # Собираю HTML через MkDocs

3. ПРОВЕРКА
   ├─ make docs-serve   # Просматриваю локально
   └─ http://localhost:8000

4. ПУБЛИКАЦИЯ
   ├─ git add docs/
   ├─ git commit -m "Update docs"
   └─ git push
```

---

## ✅ Требования для работы

### Обязательные
- ✅ Python 3.8+
- ✅ Исходный код с Doxygen комментариями
- ✅ Makefile с targets

### Опциональные (для полного функционала)
- 🟡 `doxygen` — для `make docs-doxygen`
- 🟡 `mkdocs` + `mkdocs-material` — для `make docs`
- 🟡 MkDocs plugins (в `requirements-docs.txt`)

### Установка всего
```bash
# Python зависимости
python3 -m venv .venv-docs
.venv-docs/bin/pip install -r requirements-docs.txt

# Doxygen (Linux)
sudo apt install doxygen

# Или через Makefile
make docs  # Проверит автоматически
```

---

## 🚀 Быстрый старт

### Вариант 1: Только Markdown (10 сек)
```bash
make docs-md
# Результат: обновлены docs_src/*.md
```

### Вариант 2: С Doxygen (30 сек)
```bash
make docs-doxygen
# Результат: обновлены README и docs_doxygen/
```

### Вариант 3: Полная генерация (2 мин)
```bash
make docs
# Результат: весь сайт в docs/index.html
```

### Вариант 4: Live server (для разработки)
```bash
make docs-serve
# Открыть: http://localhost:8000
# Live reload при изменении файлов
```

---

## 🔍 Что проверяет система

### 1. Исходный код (Core/Src, Core/Inc)

✅ Ищет:
```c
/**
 * @file    crypto_wallet.c
 * @brief   trezor-crypto integration: RNG, BIP-39, BIP-32, ECDSA secp256k1
 * @details Requires trezor-crypto (github.com/trezor/trezor-crypto)
 */
```

📝 Генерирует:
- `docs_src/reference-code.md` (перечень модулей)
- `docs_doxygen/html/` (полная документация)
- `README.md` (Project Structure таблица)

---

### 2. Ручная документация (docs_src/*.md)

✅ Ищет:
```markdown
## Краткий обзор

Это краткое описание модуля...
```

📝 Генерирует:
- Таблица индекса в README
- Навигация в MkDocs сайте

---

### 3. Тестовые сценарии

✅ `scripts/test_plan_signing_rng.py` генерирует:
- `docs_src/testing-plan-signing-rng.md`

---

## 📈 Статистика

| Метрика | Значение |
|---------|----------|
| **Скрипты автогенерации** | 4 основных |
| **Make targets** | 5 для документации |
| **Входных форматов** | 3 (C, Markdown, Python) |
| **Выходных форматов** | 3 (HTML, XML, Markdown) |
| **Поддерживаемых языков** | Множество (через i18n) |

---

## 🔗 Связанные файлы

| Файл | Назначение |
|------|-----------|
| `Doxyfile` | Конфигурация Doxygen |
| `Makefile` | Build targets (строки 139-178) |
| `requirements-docs.txt` | Python зависимости для MkDocs |
| `mkdocs.yml` | Конфигурация MkDocs |
| `scripts/generate_code_reference_md.py` | Генератор reference-code.md |
| `scripts/update_readme.py` | Обновитель README |
| `scripts/update_docs_src_index.py` | Индекс docs_src |
| `scripts/test_plan_signing_rng.py` | Тестовый план |

---

## ⚙️ Автоматизация в CI/CD

### Рекомендуемая настройка (GitHub Actions)

```yaml
name: Generate Docs
on: [push, pull_request]

jobs:
  docs:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - uses: actions/setup-python@v2
      - run: pip install -r requirements-docs.txt
      - run: sudo apt install doxygen
      - run: make docs-doxygen
      - run: make docs
      - uses: actions/upload-artifact@v2
        with:
          name: docs
          path: docs/
```

---

## 🎯 Рекомендации

### Для разработчиков
1. ✅ Добавляйте `@brief` и `@details` в каждый `.c` файл
2. ✅ Используйте `make docs-md` перед коммитом
3. ✅ Проверяйте `make docs-serve` локально

### Для CI/CD
1. ✅ Запускайте `make docs-doxygen` в pipeline
2. ✅ Проверяйте наличие всех комментариев
3. ✅ Публикуйте результаты в `docs/`

### Для документации
1. ✅ Используйте `## Краткий обзор` в .md файлах
2. ✅ Поддерживайте структуру docs_src/
3. ✅ Обновляйте вручную написанные разделы

---

## 📌 Текущее состояние

✅ **Система полностью настроена и работает**

- Doxygen конфигурирован в Doxyfile
- Python скрипты готовы к использованию
- Make targets интегрированы в Makefile
- MkDocs Material настроен
- Все зависимости задокументированы

---

**Дата анализа:** 2026-03-19  
**Версия:** 1.0  
**Статус:** ✅ Полностью опционально
