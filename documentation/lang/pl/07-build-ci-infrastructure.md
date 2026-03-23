# 7. Build, CI i infrastruktura

## 7.1 Repozytoria sąsiednie (nie submoduły)

Makefile oczekuje **STM32CubeH7**, **stm32_secure_boot** i **stm32-ssd1306** jako sąsiadów tego repozytorium **lub** pod wspólnym rodzicem:

```bash
export CRYPTO_DEPS_ROOT=/data/projects   # katalog nadrzędny tych trzech ścieżek
make CRYPTO_DEPS_ROOT="$CRYPTO_DEPS_ROOT" clean all
```

**Dlaczego:** cache CI często nie ma `../STM32CubeH7`; jawny `CRYPTO_DEPS_ROOT` naprawia ścieżki HAL (w tym źródła USB HAL).

## 7.2 Typowe flagi make

Patrz nagłówek `Makefile`: `USE_LWIP`, `USE_CRYPTO_SIGN`, `USE_WEBUSB`, `USE_RNG_DUMP`, `USE_TEST_SEED` itd.

## 7.3 Gitea Actions

- **Workflow:** `.gitea/workflows/simple-ci.yml`.
- **Etykieta runnera:** self-hosted `cryptowallet-host` (komentarze w workflow — unikać literówki `container: false` na części runnerów).
- **Concurrency:** `cryptowallet-stlink-…`, aby dwa joby nie walczyły o jeden ST-LINK/UART.

## 7.4 Lokalny act / nektos

Przekaż `-e CRYPTO_DEPS_ROOT=/data/projects` (lub zamontuj zależności), aby `make` widział HAL poza cache’owanym checkoutem.

## 7.5 Kontenery (opcjonalny dev)

**Katalog:** `infra/` — `docker-compose.yml`, szablony env. Typowo: Gitea + runner na hoście lab; **nie** wymagane do kompilacji firmware na gołym metalu.

Trwałość (baza Gitea, repozytoria, tokeny runnera) zależy od volume/bind-mount — `infra/.env` traktuj jako **lokalne**, bez commitowania sekretów.

## 7.6 Zależności Python do testów

```bash
pip install -r requirements-test.txt
```

Opcjonalnie: `./scripts/install-test-deps.sh` instaluje pakiety systemowe (np. `dieharder`) i zależności Python. Używane przez `pytest` i skrypty podpisu/RNG na hoście.

## 7.7 Flash i reset

Szczegóły operacyjne (st-flash, OpenOCD, NRST) są w komentarzach w kodzie i `scripts/` — na co dzień preferuj `make flash` i skrypty przechwytu UART z rozdziału 6.
