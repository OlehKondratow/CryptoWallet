# Włączanie RNG Dump w Oprogramowaniu CryptoWallet

## Szybkie Rozwiązanie

Test przechwytywania RNG nie powiedzie się, ponieważ **oprogramowanie nie jest skonfigurowane do wyprowadzania surowych danych RNG na UART**.

Musisz **przebudować oprogramowanie z flagą `USE_RNG_DUMP=1`**.

## Przewodnik Krok po Kroku

### 1. Sprawdź Aktualny Makefile

```bash
cd /data/projects/CryptoWallet
grep "USE_RNG_DUMP" Makefile
```

**Oczekiwane**: Puste wyjście (obecnie nie ustawione)

### 2. Dodaj RNG_DUMP do Makefile

Dodaj tę linię po definicji `USE_TEST_SEED` (około linia 59):

```makefile
# USE_RNG_DUMP=1: output raw RNG data on UART (for Dieharder testing)
# WARNING: Disables normal UART output - only binary RNG data sent
USE_RNG_DUMP ?= 0
```

#### Lokalizacja w Makefile:

Znajdź tę sekcję (linie ~57-67):

```makefile
# USE_TEST_SEED=1: test mnemonic "abandon...about" for dev (NEVER for real funds)
# Implies USE_CRYPTO_SIGN=1
USE_TEST_SEED ?= 0
ifeq ($(USE_TEST_SEED),1)
USE_CRYPTO_SIGN := 1
```

Dodaj swoją linię po tym, przed blokiem USE_CRYPTO_SIGN.

### 3. Dodaj Flagę do CFLAGS

Znajdź sekcję CFLAGS gdzie USE_TEST_SEED jest dodawany (około linia 60-70) i dodaj:

```makefile
ifeq ($(USE_RNG_DUMP),1)
CFLAGS += -DUSE_RNG_DUMP
endif
```

To powinno być **po** bloku `USE_TEST_SEED` i **przed** blokiem `USE_CRYPTO_SIGN`.

### 4. Przebuduj Oprogramowanie

```bash
cd /data/projects/CryptoWallet

# Opcja A: Pełne czyszczenie i przebudowa z flagą
make clean
make USE_RNG_DUMP=1 -j$(nproc)

# Opcja B: Jeśli chcesz tylko RNG dump (bez podpisywania kryptograficznego)
make clean
make USE_RNG_DUMP=1 -j$(nproc)

# Opcja C: Z kryptem i RNG dump
make clean
make USE_CRYPTO_SIGN=1 USE_RNG_DUMP=1 -j$(nproc)
```

### 5. Wgraj do Urządzenia

```bash
# To zależy od Twojej konfiguracji. Typowe opcje:

# Opcja A: STM32 ST-Link
make flash

# Opcja B: DFU (jeśli używasz bootloadera DFU)
make dfu

# Opcja C: Ręczne przy użyciu st-flash
st-flash write build/*.elf 0x08000000

# Opcja D: Przy użyciu OpenOCD
openocd -f interface/stlink-v2-1.cfg -f target/stm32h7x.cfg \
  -c "program build/*.elf verify reset exit"
```

**Uwaga**: Sprawdź cel flash w Twoim projekcie:
```bash
grep -n "^flash:" Makefile
```

### 6. Weryfikuj Wgrany Firmware

Po wgraniu urządzenie powinno wyprowadzać **tylko dane binarne** na UART (bez tekstu):

```bash
# Szybki test (zatrzymaj Ctrl+C)
python3 -m serial.tools.miniterm /dev/ttyACM0 115200 --raw

# Powinieneś zobaczyć binarne bzdury - surowe dane RNG
# NIE: wyjście tekstowe, monity menu, ani nic
```

## Integracja Kodu Oprogramowania

Kod oprogramowania powinien już mieć funkcjonalność RNG dump. Sprawdź:

```bash
# Szukaj USE_RNG_DUMP w źródle
grep -r "USE_RNG_DUMP" /data/projects/CryptoWallet/Core/

# Powinien znaleźć konfigurację UART/tasków, która:
# - Wyprowadza surowe bajty RNG gdy włączone
# - Omija normalne wiadomości UART
```

Jeśli nie znaleziono, może być konieczne dodanie go do `Core/Src/main.c` lub zadania UART.

## Typowe Problemy

### Problem 1: "make: target flash not found"
Twój projekt może używać innej metody wgrywania. Sprawdź:
```bash
make help | grep -i flash
```

### Problem 2: "make: command not found"
Musisz zainstalować ARM embedded toolchain:
```bash
sudo apt install build-essential arm-none-eabi-gcc
```

### Problem 3: "No such file or directory: stm32cubeh7"
Zależności nie znalezione. Sprawdź:
```bash
ls -d ../STM32CubeH7 ../stm32-ssd1306 ../stm32_secure_boot
```

Jeśli brakuje, sklonuj je:
```bash
cd /data/projects
git clone https://github.com/STMicroelectronics/STM32CubeH7
# itd.
```

## Testowanie Po Budowaniu

Po wgraniu oprogramowania, przetestuj RNG dump:

```bash
# Test 1: Surowy odczyt
dd if=/dev/ttyACM0 bs=1 count=100 2>/dev/null | xxd

# Test 2: Uruchom skrypt testowy
source .venv-test/bin/activate
python3 scripts/test_rng_signing_comprehensive.py --mode rng
```

Test powinien teraz:
1. Otworzyć port szeregowy ✓
2. Wykryć dostępne dane ✓
3. **Pomyślnie odczytać dane binarne** ✓

## Oczekiwane Wyjście Budowania

```
Compiling Core/Src/main.c ...
Compiling Core/Src/task_rng.c ...
Linking ...
Creating binary ...
arm-none-eabi-objcopy -O binary build/firmware.elf build/firmware.bin
Firmware ready: build/firmware.elf (128 KB)
```

## Weryfikacja

Po wgraniu, zweryfikuj flagi budowania oprogramowania:

```bash
# Sprawdź czy binarny zawiera flagę w stringach
arm-none-eabi-strings build/firmware.elf | grep -i rng

# Lub sprawdź datę budowania
ls -l build/firmware.elf
# Powinno pokazać ostatnią godzinę
```

## Powrót (Powrót do Normalnego Oprogramowania)

Aby wrócić do normalnego oprogramowania bez RNG dump:

```bash
cd /data/projects/CryptoWallet
make clean
make -j$(nproc)  # Brak flag - używa domyślnych
make flash
```

To będzie przebudować z:
- `USE_RNG_DUMP=0` (normalne wyjście UART)
- `USE_LWIP=1` (Sieć LwIP, domyślnie)
- `USE_CRYPTO_SIGN=0` (brak podpisywania, chyba że go ustawisz)

## Następne Kroki

1. Edytuj Makefile i dodaj obsługę USE_RNG_DUMP
2. Przebuduj: `make clean && make USE_RNG_DUMP=1 -j$(nproc)`
3. Wgraj do urządzenia: `make flash`
4. Weryfikuj: `python3 -m serial.tools.miniterm /dev/ttyACM0 115200 --raw`
5. Testuj: `python3 scripts/test_rng_signing_comprehensive.py --mode rng`

---

**Zobacz Również:**
- `TROUBLESHOOT_RNG_CAPTURE.md` - Pełny przewodnik rozwiązywania problemów
- `docs_src/testing/rng_capture_testing.md` - Kompletny przewodnik testowania
- Dokumentacja oprogramowania w `docs_src/crypto/`
