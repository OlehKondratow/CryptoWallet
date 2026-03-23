# Scripts (tests & utilities)

**Repository documentation index:** `README.md` at the project root (maps firmware sources, these scripts, and `docs_src/`).

All Python helpers live here.

## Python environment (venv)

From the **repository root**:

```bash
python3 -m venv .venv-scripts
.venv-scripts/bin/pip install -U pip

# Flexible dependency ranges (recommended for day-to-day)
.venv-scripts/bin/pip install -r scripts/requirements.txt

# Or exact pins (full package list — reproducible)
.venv-scripts/bin/pip install -r scripts/requirements.lock.txt
```

Activate (optional):

```bash
source .venv-scripts/bin/activate   # Linux / macOS
# .venv-scripts\Scripts\activate    # Windows cmd
```

Then run scripts as `python3 scripts/...` or `python scripts/...` from the repo root.

### Installed packages (pip)

| Package   | Role                          |
|-----------|-------------------------------|
| **pyusb** | WebUSB (`test_usb_sign.py`)   |
| **pyserial** | UART RNG capture (`capture_rng_uart.py`), CI boot log wait (`ci/uart_wait_boot_log.py`) |

There are no extra transitive dependencies for these versions; `requirements.lock.txt` lists the same two packages with pinned versions.

### CI / HIL: ожидание строк UART при загрузке

**`scripts/ci/uart_wait_boot_log.py`** — читает порт и ждёт подстрок из **`scripts/ci/uart_boot_markers.txt`** **по порядку** (LwIP, HTTP, DHCP, SNTP и т.д.). Используется в `.gitea/workflows/simple-ci.yml` (job «Analyse UART Log»).

```bash
# Локально после прошивки (порт по умолчанию /dev/ttyACM0):
python3 scripts/ci/uart_wait_boot_log.py --timeout 120
# Переменные: CI_UART_PORT, CI_UART_BOOT_TIMEOUT_SEC, CI_UART_MARKERS_FILE, CI_UART_SKIP_NO_DEVICE=1
```

Если платы нет, при `CI_UART_SKIP_NO_DEVICE=1` скрипт завершается с 0 и пишет `SKIP` в `uart_boot_status.txt`.

**Формат строк в прошивке:** `Core/Inc/app_log.h` — уровни `[ERR]`, `[WARN]`, `[INFO]`, опционально `[DBG]` (`make APP_LOG_ENABLE_DBG=1`). Подсистемы в тексте: `[MAIN]`, `[NET]`, `[SIGN]`, `[DISP]`, `[ETH]`, `[DHCP]`, `[TIME]`, `[HTTP]`, `[USER]`, `[IO]`, `[USB]`.

**Not installed via pip:** `libusb-1.0`, udev rules (WebUSB), `dieharder`, `st-flash`, toolchain — see sections below and the main `README.md`.

---

## Boot + signing smoke test

**`bootloader_secure_signing_test.py`** — build `minimal-lwip` with `USE_CRYPTO_SIGN=1` and `USE_TEST_SEED=1`, flash via `st-flash`, then verify signing over HTTP or WebUSB. Single image at **0x08000000**; for a separate secure bootloader see sibling repo **`stm32_secure_boot`**.

```bash
python3 scripts/bootloader_secure_signing_test.py
python3 scripts/bootloader_secure_signing_test.py --no-build --no-flash --ip 192.168.0.10
python3 scripts/bootloader_secure_signing_test.py --webusb
# pyserial is in scripts/requirements.txt (venv above)
python3 scripts/bootloader_secure_signing_test.py --uart-wait /dev/ttyACM0
python3 scripts/bootloader_secure_signing_test.py --elf-audit-only --no-build --no-flash --no-sign
```

---

## MVP: CWUP UART (HIL) + host unit tests

**`test_cwup_mvp.py`** — после загрузки прошивки **без** `USE_RNG_DUMP` ждёт `CW+READY`, затем в каждом раунде: `AT+PING`, `AT+CWINFO?`, `AT+READY?`, **`AT+WALLET?` / `AT+SELFTEST?` / `AT+ECHO=`** (лаборатория, без аутентификации), опционально `AT+FWINFO?` (с `--bin`), `AT+BOOTCHAIN?`. Ожидания для `AT+WALLET?`: `--expect-wallet-seed` / `--expect-crypto-sign` или env `CWUP_EXPECT_WALLET_SEED`, `CWUP_EXPECT_CRYPTO_SIGN`. Параметр **`--stress-extra-rounds N`** повторяет полную последовательность ещё N раз (нагрузка на очередь CWUP). Опционально `--bin build/cryptowallet.bin`: сверка CRC с файлом (как `fw_integrity_check.py`).

```bash
pip install pyserial   # или venv из раздела выше
python3 scripts/test_cwup_mvp.py --port /dev/ttyACM0
python3 scripts/test_cwup_mvp.py --port /dev/ttyACM0 --bin build/cryptowallet.bin
python3 scripts/test_cwup_mvp.py --port /dev/ttyACM0 --stress-extra-rounds 200 --stress-delay-ms 2
# В CI без платы:
CWUP_SKIP_NO_DEVICE=1 python3 scripts/test_cwup_mvp.py
```

**`secure_boot_image.py`** — разбор и проверка на ПК подписанного **app**-образа в формате **`stm32_secure_boot`** (`sign_image.py`, ECDSA secp256k1, не X.509). Юнит-тесты микропорчи: `tests/mvp/test_secure_boot_sign_tamper.py` (зависимость `ecdsa`).

```bash
.venv-test/bin/pip install -r requirements-test.txt
python3 scripts/secure_boot_image.py /path/to/signed_app.bin --privkey-pem ../stm32_secure_boot/scripts/root_private_key.pem
```

**Юнит-тесты (pytest):** из корня репозитория, с `pytest` из `requirements-test.txt` / `.venv-test`:

```bash
python3 -m pytest tests/mvp -q
```

**Локально как в CI (build job):** после `make` — `fw_integrity_check` + те же pytest:

```bash
./scripts/ci_host_mvp.sh
```

В **Gitea** отдельные шаги: `FW integrity — host CRC/size`, затем `MVP host tests (pytest)` (см. `.gitea/workflows/simple-ci.yml`).

---

## WebUSB signing

Default firmware build (`make` / `minimal-lwip`) enables **WebUSB** (`USE_WEBUSB=1`) together with **CWUP** on UART when `USE_RNG_DUMP=0`; disable USB with `USE_WEBUSB=0` if needed.

**`test_usb_sign.py`** — see file docstring; needs **pyusb** and udev rules (`udev/99-cryptowallet-webusb.rules`).

---

## Test plan (Markdown) & DIEHARDER / RNG

**`test_plan_signing_rng.py`** — prints or writes the signing + RNG test plan:

```bash
python3 scripts/test_plan_signing_rng.py
python3 scripts/test_plan_signing_rng.py --write docs_src/testing-plan-signing-rng.md
```

**`capture_rng_uart.py`** — UART → raw `rng.bin` for statistical tests (firmware must output **binary only**):

```bash
python3 scripts/capture_rng_uart.py --port /dev/ttyACM0 --out rng.bin --bytes 134217728
```

**`run_dieharder.py`** — wrapper for `dieharder -g 201 -f <file>`:

```bash
sudo apt install dieharder
python3 scripts/run_dieharder.py --file rng.bin
python3 scripts/run_dieharder.py --list-tests
```

| Script | Role |
|--------|------|
| `test_cwup_mvp.py` | CWUP HIL: PING/CWINFO/READY/FWINFO/BOOTCHAIN, stress rounds |
| `mvp_cwup.py` | Helpers for CWUP parsing (used by tests) |
| `secure_boot_image.py` | Parse/verify stm32_secure_boot signed `.bin` on host |
| `bootloader_secure_signing_test.py` | Build, flash, HTTP/WebUSB signing smoke test |
| `test_usb_sign.py` | WebUSB ping / sign |
| `test_plan_signing_rng.py` | Generate test plan + DIEHARDER checklist |
| `capture_rng_uart.py` | Capture RNG stream to file |
| `run_dieharder.py` | Run dieharder on capture |
| `generate_code_reference_md.py` | Build `docs_src/reference-code.md` from C/Python file headers (`make docs-code-md`) |
| `update_readme.py` | Parse Doxygen XML, update README “Project Structure” table, emit `docs_doxygen/md/*.md` |

**Doxygen (brief/details → README + Markdown):** run `doxygen Doxyfile` then `python3 scripts/update_readme.py` or `make docs-doxygen`. See `docs_src/doxygen-comments.md` for @brief / @details conventions.

---

## Docs

- Signing: `docs_src/testing-signing.md`, `docs_src/verification-signing.md`
- Test plan (generated): `docs_src/testing-plan-signing-rng.md`
- Code reference (generated from headers): `docs_src/reference-code.md` — `make docs-code-md`
- Pipeline description: `docs_src/code-doc-generation.md`
- RNG: `docs_src/rng-entropy.md`
