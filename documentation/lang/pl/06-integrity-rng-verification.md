# 6. Integralność, RNG i weryfikacja

## 6.1 Integralność firmware w czasie działania (`fw_integrity`)

**Cel:** porównać CRC32 i długość **działającego** obszaru aplikacji z oczekiwaniami — wykryć uszkodzenie lub zły obraz. **To nie** zamiennik verified boot.

| Element | Lokalizacja |
|---------|-------------|
| Implementacja | `Core/Src/fw_integrity.c`, `Core/Inc/fw_integrity.h` |
| Region | Symbole linkera `__app_flash_start` / `__app_flash_end` |
| Linia logu | `FWINFO crc32=…,len=…` przy starcie |
| Sprawdzenie na hoście | `scripts/fw_integrity_check.py` na `build/cryptowallet.bin`; opcjonalnie `--expect-fwinfo-line` z UART |

**CWUP:** `AT+FWINFO?` zwraca ten sam łańcuch przy aktywnym CWUP (`USE_RNG_DUMP=0`).

## 6.2 Dwa tryby TRNG na UART (nie mieszać)

| Tryb | Build | Zawartość UART |
|------|-------|----------------|
| Surowy zrzut | `USE_RNG_DUMP=1` | Ciągłe binarne bajty TRNG — **CWUP wyłączony** |
| Ramkowy (spec) | CWUP §7 | `AT+RNG=START` potem ramki — w firmware **jeszcze nie zaimplementowano** |

`scripts/capture_rng_uart.py` przechwytuje **surowe** bajty do analizy w stylu dieharder przy `USE_RNG_DUMP=1`.

**Użycie kryptograficzne vs ta ścieżka przechwytu:** zrzut UART służy do **statystycznej weryfikacji** TRNG. **Provisioning** przyszłego zaszyfrowanego blobu portfela (sole, nonce AEAD) musi używać **tego samego sprzętowego RNG w firmware**, nie pliku hosta z capture — patrz **sekcja 3.5** [`03-cryptography-and-signing.md`](03-cryptography-and-signing.md) (rola TRNG, wiązanie UID, zakres `USE_RNG_DUMP`).

## 6.3 Testy na hoście (bez płyty)

- **`pytest tests/mvp`** — CRC/build, parsowanie FWINFO, logika manipulacji obrazem secure boot (`ecdsa` z `requirements-test.txt`).
- **`scripts/fw_integrity_check.py`** — krok CI „FW integrity”.

## 6.4 Płyta / CI

- **Gitea:** `.gitea/workflows/simple-ci.yml` — build, flash, analiza UART, **przechwyt RNG** gdy `CI_BUILD_USE_RNG_DUMP=1` i nie pominięto.
- **Zmienne:** `CRYPTO_DEPS_ROOT` (sąsiednie `STM32CubeH7`, `stm32_secure_boot`, `stm32-ssd1306`), `CI_RNG_CAPTURE_BYTES` (domyślnie duży przechwyt do statystyk), `CI_RNG_UART_CAPTURE_STRICT=0|1` (czy nieudany przechwyt failuje job), `CI_SKIP_RNG_UART_CAPTURE`.

## 6.5 Dieharder (szybki smoke)

CI może uruchomić `dieharder` na przechwyconym pliku. **Kod wyjścia 0** jest normalny nawet gdy pojedynczy wiersz pokazuje **`FAILED`** — to jedna statystyka, nie awaria procesu. Krótkie pliki są **wielokrotnie przewijane**; **wartości p nie są interpretowalne jak dla nieskończonego strumienia i.i.d.** Do poważnej oceny: duży przechwyt (setki MiB) i rozszerzony dieharder offline.

## 6.6 Lista kontrolna ręczna

1. Build → `build/cryptowallet.bin`.
2. Flash → log UART zawiera `FWINFO`.
3. `python3 scripts/fw_integrity_check.py build/cryptowallet.bin` — dopasuj CRC/długość do logu urządzenia lub `AT+FWINFO?` gdy dotyczy.
