# 5. UART — CWUP-0.1 (protokół CryptoWallet UART)

**Implementacja:** `Core/Src/cwup_uart.c`. **Typowy build produktowy:** `USE_WEBUSB=1`, `USE_RNG_DUMP=0` — CWUP na USART3 razem z WebUSB na PA11/PA12.

**Zakres:** diagnostyka lab, CI oraz (w specyfikacji) ramkowy TRNG. **Nie** aktualizacja firmware przez UART (to bootloader). **Nie** zamiennik HTTP/WebUSB jako głównego kanału użytkownika.

## 5.1 Kontekst łańcucha bootowania

```
ROM → (opcjonalnie stm32_secure_boot) → aplikacja CryptoWallet
```

CWUP istnieje **dopiero po** starcie FreeRTOS. Ruch podczas bootloadera **nie jest** CWUP.

## 5.2 Fazy linii

| Faza | Znaczenie | Widok hosta |
|------|-----------|-------------|
| A | Przed gotowością CWUP | Może być tekst; CWUP niegotowy |
| B | `CW+READY,proto=CWUP/0.1,...` | CWUP ogłoszony |
| C | Tryb TEKST: linie `AT+` / `CW+` | Polecenia wg §5.4 |
| D | Binarny ramkowy TRNG (§5.5) | Po `AT+RNG=START` — w firmware **nie zaimplementowano** |

`rng=` w `CW+READY` dotyczy **ramkowego** TRNG — **nie** surowego strumienia `USE_RNG_DUMP`.

## 5.3 Tryby

| Tryb | TX | RX |
|------|----|----|
| TEXT | Linie `\r\n` | `AT+...` |
| BINARY_RNG | Tylko ramki §5.5 | `AT+RNG=STOP` / timeout |

Reguła: brak logów `printf` na tym samym UART w BINARY_RNG.

## 5.4 Polecenia (MVP)

Format: `AT+<CZASOWNIK>[=<args>]\r\n` → `CW+<KOD>[,payload]\r\n` lub `CW+ERR=<n>,<msg>\r\n`.

| Polecenie | Cel |
|-----------|-----|
| `AT+CWINFO?` | Protokół + informacje o buildzie |
| `AT+FWINFO?` | Łańcuch integralności aplikacji (jak `fw_integrity_snprint()` / log `FWINFO`) |
| `AT+BOOTCHAIN?` | Łańcuch bootowania dla człowieka (lab) |
| `AT+MARKS` | Markery CI — **nie zaimplementowano** |
| `AT+RNG=START,<bytes>` / `AT+RNG=STOP` | Sesja ramkowego TRNG — **nie zaimplementowano** |
| `AT+PING` | `CW+PONG` |
| `AT+READY?` | Powtórz `CW+READY` |
| `AT+WALLET?` | Lab: flagi `seed` / `crypto_sign` — **bez sekretów** |
| `AT+SELFTEST?` | Tick / sanity |
| `AT+ECHO=<text>` | Drukowalny ASCII — regresja CI |

Zapisy portfela pozostają na HTTP/WebUSB — nie CWUP-0.1.

## 5.5 Format ramki TRNG (specyfikacja; parser hosta)

```
[0..1]   magic 0xC7 0x57
[2..3]   u16 payload_len N (LE, N ≤ 256)
[4..3+N] N bajtów TRNG
[4+N..7+N] u32 CRC32 (IEEE) nad [0 .. 4+N-1]
```

## 5.6 Status implementacji (firmware)

| Element | Status |
|---------|--------|
| `CW+READY`, `AT+PING`, `AT+CWINFO?`, `AT+FWINFO?`, `AT+BOOTCHAIN?`, `AT+WALLET?`, `AT+SELFTEST?`, `AT+ECHO=` | Zaimplementowano |
| `AT+MARKS`, ramkowy TRNG §5.5 + `AT+RNG=*` | **Nie zaimplementowano** |
| `USE_RNG_DUMP=1` | Surowy binarny UART — **Cwup_Init** nie startuje CWUP |

## 5.7 Mapowanie skryptów

| Skrypt | Rola |
|--------|------|
| `uart_wait_boot_log.py` | Oczekiwanie na `CW+READY` / markery w trybie tekstowym |
| `scripts/test_cwup_mvp.py` | Testy HIL AT |
| `scripts/capture_rng_uart.py` | Surowy przechwyt bajtów w trybie **dump** — nie parser §5.5 dziś |
