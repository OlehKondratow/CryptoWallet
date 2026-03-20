# Konfiguracja Środowiska Testowania

## ✅ Ukończone Prace

Wszystkie zależności Pythona zostały zainstalowane i skonfigurowane dla pakietu testów CryptoWallet.

### Pliki Utworzone

| Plik | Rozmiar | Cel |
|------|---------|-----|
| `requirements-test.txt` | 696 B | Specyfikacje pakietów Pythona z zakresami wersji |
| `requirements-test-lock.txt` | 705 B | Dokładne wersje wszystkich zainstalowanych pakietów |
| `install-test-deps.sh` | 3.4 K | Zautomatyzowany instalator zależności systemowych i Pythona |
| `INSTALL_DIEHARDER.txt` | 2.6 K | Szybki odnośnik dla instalacji dieharder |
| `docs_src/INSTALL_TEST_DEPS.md` | ~7 K | Kompletny przewodnik instalacji z rozwiązywaniem problemów |

### Zainstalowane Pakiety Pythona (22 total)

#### Zależności Podstawowe (Wymagane)
- **pyserial 3.5** - Komunikacja portów szeregowych z urządzeniem CryptoWallet
- **requests 2.32.5** - Klient HTTP do komunikacji REST API
- **colorama 0.4.6** - Kolorowy wynik terminala

#### Narzędzia Programistyczne (Opcjonalne ale rekomendowane)
- **pytest 7.4.4** - Framework testowania jednostkowego
- **black 24.10.0** - Formatter kodu
- **flake8 7.3.0** - Linter kodu

#### Zarządzanie Budowaniem i Pakietami
- **setuptools 82.0.1**, **pip 26.0.1**, **wheel 0.46.3**

#### Zależności Przechodzące
- charset-normalizer, idna, urllib3 (obsługa HTTP)
- click, pathspec, platformdirs (narzędzia CLI)
- I inne dla lintingu/formatowania

## 🐛 Rozwiązany Problem

**Problem:** `pytest-serial>=0.0.9` - Ten pakiet nie istnieje w PyPI

**Przyczyna główna:** Błędna specyfikacja pakietu w oryginalnych wymaganiach

**Rozwiązanie:** Usunięto nieistniejący pakiet, zachowano tylko ważne zależności

## 🚀 Szybki Start

### 1. Zainstaluj Zależności Systemowe

**Automatycznie (Rekomendowane):**
```bash
cd /data/projects/CryptoWallet
./install-test-deps.sh
```

**Ręcznie (Wybierz Swoją OS):**

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

### 2. Aktywuj Wirtualne Środowisko

```bash
source .venv-test/bin/activate
```

### 3. Weryfikuj Instalację

```bash
# Sprawdź dieharder
dieharder --version

# Sprawdź pakiety Pythona
python3 -c "import serial, requests, colorama, pytest; print('✓ All packages OK')"

# Testuj skrypt testu
python3 scripts/test_rng_signing_comprehensive.py --help
```

## 📖 Dokumentacja

- **Szczegóły instalacji**: `docs_src/INSTALL_TEST_DEPS.md`
- **Konfiguracja wirtualnego środowiska**: `docs_src/VENV_SETUP.md`
- **Przewodnik testowania**: `docs_src/TESTING_GUIDE_RNG_SIGNING.md`
- **Szybki start**: `VENV_QUICKSTART.txt`
- **Wymagania**: `requirements-test.txt` i `requirements-test-lock.txt`

## 🎯 Dostępne Tryby Testu

Po skonfigurowaniu wszystkiego możesz uruchamiać:

```bash
# Test RNG (przechwytywanie i analiza entropii)
python3 scripts/test_rng_signing_comprehensive.py --mode rng

# Analiza Dieharder (testy statystyczne)
python3 scripts/test_rng_signing_comprehensive.py --mode dieharder

# Test Podpisywania Transakcji
python3 scripts/test_rng_signing_comprehensive.py --mode signing --ip 192.168.0.10

# Kompleksowy Pakiet Testów
python3 scripts/test_rng_signing_comprehensive.py --mode verify-all
```

## ✅ Kontrolna Lista Weryfikacji

- [ ] `dieharder --version` pokazuje numer wersji
- [ ] `.venv-test/bin/python3 --version` pokazuje Python 3.12
- [ ] `python3 -c "import serial"` powiedzie się (bez ImportError)
- [ ] `python3 -c "import requests"` powiedzie się
- [ ] `python3 scripts/test_rng_signing_comprehensive.py --help` pokazuje pełną pomoc
- [ ] Urządzenie USB/UART jest podłączone (dla `--mode rng`)

## 📝 Pliki Wymagań

### requirements-test.txt
Zawiera elastyczne ograniczenia wersji dla programowania:
- Używaj gdy: Instalowanie w nowym środowisku, elastyczne wersje akceptowalne
- Instaluj z: `pip install -r requirements-test.txt`

### requirements-test-lock.txt
Zawiera dokładnie przypiętych wersji dla reprodukowalności:
- Używaj gdy: Dokładna reprodukcja potrzebna, wdrożenie do produkcji
- Instaluj z: `pip install -r requirements-test-lock.txt`
- Wygeneruj: `pip freeze > requirements-test-lock.txt`

## 🐛 Rozwiązywanie Problemów

### "dieharder: command not found"
```bash
# Sprawdzić czy zainstalowany
which dieharder

# Zainstaluj dla twojej OS (zobacz sekcję Szybki Start)
```

### "ModuleNotFoundError: No module named 'serial'"
```bash
# Aktywuj venv
source .venv-test/bin/activate

# Przeinstaluj pakiety
pip install -r requirements-test.txt

# Weryfikuj
python3 -c "import serial; print('OK')"
```

### "Permission denied" na /dev/ttyUSB0
```bash
# Dodaj użytkownika do grupy dialout (Linux)
sudo usermod -a -G dialout $USER
newgrp dialout

# Weryfikuj
ls -l /dev/ttyUSB0
```

### Dalej mają problemy?
Zobacz `docs_src/INSTALL_TEST_DEPS.md` dla kompletnego przewodnika rozwiązywania problemów.

## 📊 Informacje o Środowisku

- **Wersja Pythona**: 3.12.7
- **Wirtualne Środowisko**: `.venv-test/`
- **Pakiety Razem**: 22
- **Data Konfiguracji**: 2026-03-20
- **Status**: ✅ Gotowy

## 🔄 Aktualizacja Zależności

Aby aktualizować pakiety przy zachowaniu ograniczeń wersji:

```bash
source .venv-test/bin/activate
pip install --upgrade -r requirements-test.txt
pip freeze > requirements-test-lock.txt
```

## 📚 Powiązane Pliki

```
.
├── .venv-test/                          # Wirtualne środowisko
├── requirements-test.txt                # Specyfikacje pakietów
├── requirements-test-lock.txt           # Przyczepione wersje
├── install-test-deps.sh                 # Auto-instalator
├── INSTALL_DIEHARDER.txt               # Odnośnik Dieharder
├── activate-tests.sh                    # Pomocnik aktywacji venv
├── run-tests.sh                         # Skrypt uruchamiania testów
├── VENV_QUICKSTART.txt                 # Szybki start
├── docs_src/
│   ├── INSTALL_TEST_DEPS.md            # Kompletny przewodnik konfiguracji
│   ├── VENV_SETUP.md                   # Szczegóły venv
│   └── TESTING_GUIDE_RNG_SIGNING.md    # Przewodnik testowania
└── scripts/
    └── test_rng_signing_comprehensive.py  # Główny skrypt testowy
```

---

**Ostatnia Aktualizacja**: 2026-03-20  
**Status**: ✅ Kompletne i Zweryfikowane
