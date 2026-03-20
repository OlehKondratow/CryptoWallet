# Instalacja Dieharder

## Szybka Instalacja

Narzędzie Dieharder jest wymagane do uruchamiania testów RNG.

### Automatyczna (Rekomendowana)

```bash
./install-test-deps.sh
```

### Ręczna - Debian/Ubuntu

```bash
sudo apt update
sudo apt install -y dieharder
```

### Ręczna - Fedora/RHEL

```bash
sudo dnf install -y dieharder
```

### Ręczna - macOS

```bash
brew install dieharder
```

### Ręczna - Arch Linux

```bash
sudo pacman -S dieharder
```

## Weryfikacja

```bash
dieharder --version
```

Powinieneś zobaczyć numer wersji.

## Typowe Problemy

### "dieharder: command not found"

Jeśli dieharder nie jest znaleziony po instalacji:

1. Sprawdzić czy instalacja się powiodła
2. Wyloguj się i zaloguj ponownie
3. Spróbuj innego pakietu menedżera dla Twojej OS

### "Permission denied"

Może być konieczne użycie sudo:

```bash
sudo dieharder --version
```

## Następne Kroki

Po instalacji dieharder możesz uruchamiać testy:

```bash
source .venv-test/bin/activate
python3 scripts/test_rng_signing_comprehensive.py --mode rng
```

Aby uzyskać więcej informacji, zobacz:
- `docs_src/crypto/rng_dump_setup_pl.md` - Jak włączyć RNG Dump
- `docs_src/crypto/rng_test_checklist_pl.txt` - Kontrol lista testów
