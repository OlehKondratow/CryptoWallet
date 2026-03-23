# 7. Сборка, CI и инфраструктура

## 7.1 Соседние репозитории (не субмодули)

Makefile ожидает **STM32CubeH7**, **stm32_secure_boot** и **stm32-ssd1306** как соседей этого репозитория **или** под общим родителем:

```bash
export CRYPTO_DEPS_ROOT=/data/projects   # родитель этих трёх каталогов
make CRYPTO_DEPS_ROOT="$CRYPTO_DEPS_ROOT" clean all
```

**Зачем:** в CI кэши часто без `../STM32CubeH7`; явный `CRYPTO_DEPS_ROOT` исправляет пути HAL (в т.ч. исходники USB HAL).

## 7.2 Типичные флаги make

См. заголовок `Makefile`: `USE_LWIP`, `USE_CRYPTO_SIGN`, `USE_WEBUSB`, `USE_RNG_DUMP`, `USE_TEST_SEED` и т.д.

## 7.3 Gitea Actions

- **Workflow:** `.gitea/workflows/simple-ci.yml`.
- **Метка runner:** self-hosted `cryptowallet-host` (см. комментарии в workflow — избегать опечатки `container: false` на части runner’ов).
- **Concurrency:** `cryptowallet-stlink-…`, чтобы два job не боролись за один ST-LINK/UART.

## 7.4 Локальный act / nektos

Передавать `-e CRYPTO_DEPS_ROOT=/data/projects` (или монтировать зависимости), чтобы `make` видел HAL вне кэшированного checkout.

## 7.5 Контейнеры (опционально для разработки)

**Каталог:** `infra/` — `docker-compose.yml`, шаблоны env. Типично: Gitea + runner на лабораторном хосте; **не** обязательно для компиляции прошивки на «голом железе».

Персистентность (БД Gitea, репозитории, токены runner) зависит от volume/bind-mount — `infra/.env` считать **локальным**, секреты не коммитить.

## 7.6 Зависимости Python для тестов

```bash
pip install -r requirements-test.txt
```

Опционально: `./scripts/install-test-deps.sh` ставит системные пакеты (например `dieharder`) и Python-зависимости. Используется `pytest` и скрипты подписи/RNG на хосте.

## 7.7 Прошивка и сброс

Операционные детали (st-flash, OpenOCD, NRST) собраны в комментариях к коду и `scripts/` — в быту предпочтительны `make flash` и скрипты захвата UART из главы 6.
