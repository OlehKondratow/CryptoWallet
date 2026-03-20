# Przewodnik Rozwiązywania Problemów Przechwytywania RNG

## Napotkany Błąd

```
✗ Failed to capture RNG: device reports readiness to read but returned no data 
  (device disconnected or multiple access on port?)
```

Ten błąd występuje gdy port szeregowy otwiera się pomyślnie ale zwraca **brak danych**. Zazwyczaj oznacza to:

1. **Urządzenie nie podłączone lub odłączone**
2. **Wiele programów uzyskuje dostęp do tego samego portu** (coś już czyta)
3. **Oprogramowanie nie zbudowane z `USE_RNG_DUMP=1`**
4. **Urządzenie nie wyprowadza danych RNG na UART**

---

## Krok po Kroku Rozwiązywanie Problemów

### 1. Weryfikuj Połączenie Urządzenia

Najpierw sprawdź czy urządzenie jest fizycznie podłączone:

```bash
# Wylistuj wszystkie urządzenia USB/Serial
ls -la /dev/tty* | grep -E "(USB|ACM|FTDI|Serial)"

# Lub użyj lsusb aby zobaczyć urządzenia USB
lsusb | grep -i stm32

# Lub sprawdź dmesg dla ostatnich połączeń
dmesg | tail -20 | grep -E "usb|tty"
```

**Oczekiwane wyjście**: Powinno pokazać `/dev/ttyACM0`, `/dev/ttyUSB0`, lub podobne

**Jeśli żadne urządzenie się nie pojawia**:
- Sprawdź połączenie kabla USB do płyty STM32H743
- Spróbuj inny port USB
- Spróbuj inny kabel USB
- Zrestartuj urządzenie (wyłącz i włącz zasilanie)

---

### 2. Weryfikuj Uprawnienia Portu

Sprawdź czy masz uprawnienia dostępu do portu szeregowego:

```bash
# Sprawdź bieżące uprawnienia
ls -la /dev/ttyACM0

# Powinieneś zobaczyć coś takiego:
# crw-rw---- 1 root dialout 166, 0 ...

# Jeśli nie jesteś w grupie dialout:
sudo usermod -a -G dialout $USER
newgrp dialout

# Wyloguj się i zaloguj ponownie, następnie weryfikuj:
id | grep dialout
```

**Jeśli odmowa dostępu**:
```bash
# Tymczasowe rozwiązanie (bieżąca sesja)
sudo chmod 666 /dev/ttyACM0

# Stałe rozwiązanie (dodaj do grupy dialout - patrz wyżej)
```

---

### 3. Sprawdzaj Konflikty Portów

Inny program może być używany portu (monitor szeregowy, debugger IDE, itp.):

```bash
# Zobaczy co używa port
sudo lsof /dev/ttyACM0

# Lub sprawdzić czy proces go blokuje
fuser /dev/ttyACM0

# Typowe podejrzani do zamknięcia:
# - Arduino IDE Serial Monitor
# - VSCode/Cursor Serial Monitor
# - miniterm/picoterm
# - Inne instancje tego skryptu
```

**Jeśli port jest w użyciu**:
- Zamknij wszystkie monitory szeregowe IDE
- Zabij inne instancje testu: `pkill -f test_rng_signing_comprehensive.py`
- Czekaj 2-3 sekundy przed ponowieniem próby

---

### 4. Weryfikuj Konfigurację Oprogramowania

**Krytyczne**: Oprogramowanie **musi być zbudowane z `USE_RNG_DUMP=1`**

To konfiguruje STM32 do wyprowadzania **surowych danych RNG** na UART (tylko binarne, bez tekstu).

#### Sprawdzaj Makefile lub Konfigurację Budowania

```bash
# Szukaj ustawienia USE_RNG_DUMP
grep -r "USE_RNG_DUMP" /data/projects/CryptoWallet/

# Powinno znaleźć coś takiego:
# - Makefile: USE_RNG_DUMP=1
# - CMakeLists.txt lub konfiguracja budowania
# - Lub ustawione w Core/Src/main.h
```

#### Jeśli NIE Ustawione

Musisz **przebudować oprogramowanie** z tą flagą:

```bash
cd /data/projects/CryptoWallet

# Opcja 1: Ustaw w Makefile
sed -i 's/USE_RNG_DUMP=0/USE_RNG_DUMP=1/' Makefile

# Opcja 2: Ustaw jako zmienną środowiskową
export USE_RNG_DUMP=1

# Następnie przebuduj
make clean
make -j$(nproc)

# Wgraj do urządzenia (jeśli Twój projekt ma cel flash)
make flash
```

#### Weryfikuj Oprogramowanie

Po wgraniu, przetestuj UART ręcznie:

```bash
# Zainstaluj miniterm lub podobne
pip install pyserial

# Czytaj surowe dane UART (zatrzymaj Ctrl+C)
python3 -m serial.tools.miniterm /dev/ttyACM0 115200 --raw

# Powinieneś zobaczyć binarne bzdury (dane RNG)
# NIE: wyjście tekstowe, monity menu, lub nic
```

---

### 5. Testuj Komunikację UART Ręcznie

Przed uruchomieniem pełnego testu, zweryfikuj podstawową komunikację szeregową:

```python
#!/usr/bin/env python3
# Prosty test szeregowy
import serial
import time

port = "/dev/ttyACM0"
try:
    ser = serial.Serial(port=port, baudrate=115200, timeout=2)
    time.sleep(0.5)
    
    print(f"Port {port} otwarty pomyślnie")
    print(f"In waiting: {ser.in_waiting}")
    
    # Spróbuj odczytać trochę danych
    data = ser.read(100)
    print(f"Odczytano {len(data)} bajtów")
    print(f"Pierwsze 16 bajtów (hex): {data[:16].hex()}")
    
    ser.close()
    print("✓ Komunikacja szeregowa działa")
except Exception as e:
    print(f"✗ Błąd: {e}")
```

Zapisz jako `test_serial.py` i uruchom:
```bash
python3 test_serial.py
```

**Oczekiwane**:
- Port otwiera się pomyślnie
- `in_waiting` pokazuje dostępne dane
- Odczyta 100+ bajtów
- Pierwsze bajty są binarne (nie ASCII tekst)

---

## Szybka Kontrolna Lista Diagnostyczna

Uruchom te kontrole w kolejności:

```bash
# 1. Urządzenie podłączone?
ls /dev/ttyACM0
# Oczekiwane: /dev/ttyACM0 (lub /dev/ttyUSB0)

# 2. Czy możesz czytać z portu?
sudo dd if=/dev/ttyACM0 bs=1 count=10 2>/dev/null | xxd
# Oczekiwane: Kilka bajtów (dane binarne)

# 3. Python może go otworzyć?
python3 -c "import serial; s=serial.Serial('/dev/ttyACM0'); print('✓')"
# Oczekiwane: ✓ i bez błędu

# 4. Oprogramowanie ma USE_RNG_DUMP=1?
grep -i "USE_RNG_DUMP" /data/projects/CryptoWallet/Makefile
# Oczekiwane: USE_RNG_DUMP=1

# 5. Oprogramowanie niedawno wgrane?
ls -la /data/projects/CryptoWallet/build/*.elf
# Jeśli starsze niż oczekiwane, trzeba przebudować i wgrać
```

---

## Typowe Problemy i Rozwiązania

| Problem | Objawy | Rozwiązanie |
|---------|--------|-----------|
| **Urządzenie nie podłączone** | "No such file or directory: /dev/ttyACM0" | Sprawdź kabel USB i port |
| **Wielokrotny dostęp** | "Failed to capture RNG: no data returned" | Zamknij monitory szeregowe, zabij inne instancje |
| **Problem z oprogramowaniem** | "No data returned" (ale port się otwiera) | Przebuduj z `USE_RNG_DUMP=1` i wgraj |
| **Niezgodność prędkości transmisji** | "Garbage or no data" | Weryfikuj szybkość transmisji (115200) |
| **Odmowa dostępu** | "Permission denied" | Dodaj użytkownika do grupy dialout |
| **Inna ścieżka urządzenia** | Działa dla kogoś innego ale nie dla ciebie | Użyj `--port /dev/ttyUSB0` lub sprawdź `ls /dev/tty*` |

---

## Jeśli Dalej Nie Działa

Spróbuj tych dodatkowych kroków:

### Resetuj Urządzenie
```bash
# Cykl zasilania (odłącz i ponownie podłącz USB)
# Lub użyj reset ST-Link jeśli dostępny

# Następnie czekaj 2-3 sekundy i spróbuj ponownie
python3 scripts/test_rng_signing_comprehensive.py --mode rng
```

### Przebuduj Wszystko
```bash
cd /data/projects/CryptoWallet

# Pełne czyszczenie
make distclean 2>/dev/null || true

# Przebuduj z wyraźnym RNG dump
make USE_RNG_DUMP=1 -j$(nproc)

# Wgraj (zależy od Twojej konfiguracji)
make flash  # lub podobnie dla Twojej konfiguracji
```

### Testuj Innym Narzędziem
```bash
# Spróbuj alternatywnych narzędzi do weryfikacji szeregowych

# Opcja 1: miniterm (prosty)
python3 -m serial.tools.miniterm /dev/ttyACM0 115200 --raw

# Opcja 2: screen
screen /dev/ttyACM0 115200

# Opcja 3: picocom
sudo apt install picocom
picocom -b 115200 /dev/ttyACM0
```

Jeśli te pokazują dane binarne, urządzenie i port działają. Wtedy problem to konfiguracja oprogramowania.

### Sprawdzaj Logi
```bash
# Włącz wyjście debugowania w skrypcie testu
python3 scripts/test_rng_signing_comprehensive.py --mode rng -v 2>&1 | tee test_debug.log

# Przejrzyj log dla szczegółów
cat test_debug.log | grep -i "error\|failed\|timeout"
```

---

## Następne Kroki

1. **Uruchom kontrolę diagnostyczną powyżej** - określ który krok nie powiódł się
2. **Napraw konkretny problem** - użyj rozwiązania z tabeli
3. **Ponownie spróbuj testu**:
   ```bash
   source .venv-test/bin/activate
   python3 scripts/test_rng_signing_comprehensive.py --mode rng
   ```

4. **Jeśli dalej nie działa** - sprawdź:
   - Log urządzenia: `dmesg | tail -20`
   - Źródło oprogramowania: `/data/projects/CryptoWallet/Core/Src/` dla konfiguracji RNG
   - Wyjście budowania: Sprawdź wyjście `make` dla ostrzeżeń/błędów

---

## Wskaźniki Sukcesu

Gdy wszystko działa prawidłowo, powinnieneś zobaczyć:

```
✓ Checking prerequisites...
  ✓ pyserial
  ✓ dieharder
  ✓ serial port

RNG Data Capture from UART
ℹ Port: /dev/ttyACM0, Baud: 115200
ℹ Target: 128 MiB
ℹ Capturing RNG data...

Captured: 10 MiB (7.8%)  [████░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░] ...
Captured: 20 MiB (15.6%) [████████░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░] ...
...
Captured: 128 MiB (100%) [████████████████████████████████████████]

✓ Successfully captured 128 MiB RNG data to rng.bin
✓ Saved analysis to rng_analysis.txt
```

Przechwytywanie powinno trwać **5-15 minut** w zależności od szybkości transmisji UART i szybkości RNG urządzenia.

---

**Zobacz również:**
- `docs_src/testing/rng_dump_setup.md` - Instrukcje przebudowy oprogramowania
- `docs_src/TESTING_GUIDE_RNG_SIGNING.md` - Kompletny przewodnik testowania
- Dokumentacja oprogramowania w `docs_src/crypto/`
