# 7. Build, CI, and infrastructure

## 7.1 Sibling repositories (not submodules)

The Makefile expects **STM32CubeH7**, **stm32_secure_boot**, and **stm32-ssd1306** as siblings of this repo **or** under a common parent:

```bash
export CRYPTO_DEPS_ROOT=/data/projects   # parent of those three directories
make CRYPTO_DEPS_ROOT="$CRYPTO_DEPS_ROOT" clean all
```

**Why:** CI checkouts (e.g. act cache) often lack `../STM32CubeH7`; explicit `CRYPTO_DEPS_ROOT` fixes HAL paths (including USB HAL sources).

## 7.2 Common make flags

See `Makefile` header: `USE_LWIP`, `USE_CRYPTO_SIGN`, `USE_WEBUSB`, `USE_RNG_DUMP`, `USE_TEST_SEED`, etc.

## 7.3 Gitea Actions

- **Workflow:** `.gitea/workflows/simple-ci.yml`.
- **Runner label:** self-hosted `cryptowallet-host` (see workflow comments—avoid `container: false` typo on some runners).
- **Concurrency:** `cryptowallet-stlink-…` so two jobs do not fight over one ST-LINK/UART.

## 7.4 Local act / nektos

Pass `-e CRYPTO_DEPS_ROOT=/data/projects` (or bind-mount deps) so `make` sees HAL sources outside the cached checkout.

## 7.5 Containers (optional dev)

**Directory:** `infra/` — `docker-compose.yml`, env templates. Typical use: Gitea + runner on a lab host; **not** required to compile firmware on bare metal.

Persistence (Gitea DB, repos, runner tokens) is volume/bind-mount specific—treat `infra/.env` as **local**, never commit secrets.

## 7.6 Python test dependencies

```bash
pip install -r requirements-test.txt
```

Optional: `./scripts/install-test-deps.sh` installs system packages (e.g. `dieharder`) and Python deps. Used by `pytest` and host signing/RNG scripts.

## 7.7 Flash and reset

Operational detail (st-flash, OpenOCD, NRST) is consolidated in code comments and `scripts/`—for day-to-day, prefer `make flash` and UART capture scripts documented in chapter 6.
