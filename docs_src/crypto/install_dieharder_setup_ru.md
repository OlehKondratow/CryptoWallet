# Установка Dieharder

## Быстрая Установка

Инструмент Dieharder требуется для запуска тестов RNG.

### Автоматическая (Рекомендуется)

```bash
./install-test-deps.sh
```

### Ручная - Debian/Ubuntu

```bash
sudo apt update
sudo apt install -y dieharder
```

### Ручная - Fedora/RHEL

```bash
sudo dnf install -y dieharder
```

### Ручная - macOS

```bash
brew install dieharder
```

### Ручная - Arch Linux

```bash
sudo pacman -S dieharder
```

## Проверка

```bash
dieharder --version
```

Вы должны увидеть номер версии.

## Типичные Проблемы

### "dieharder: command not found"

Если dieharder не найден после установки:

1. Проверьте успешность установки
2. Выйдите и снова войдите
3. Попробуйте другой пакетный менеджер для вашей ОС

### "Permission denied"

Может потребоваться использование sudo:

```bash
sudo dieharder --version
```

## Следующие Шаги

После установки dieharder вы можете запускать тесты:

```bash
source .venv-test/bin/activate
python3 scripts/test_rng_signing_comprehensive.py --mode rng
```

Для получения дополнительной информации см.:
- `docs_src/crypto/rng_dump_setup_ru.md` - Как включить RNG Dump
- `docs_src/crypto/rng_test_checklist_ru.txt` - Контрольный список тестов
