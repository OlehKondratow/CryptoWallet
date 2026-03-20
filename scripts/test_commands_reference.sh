#!/bin/bash
# CryptoWallet RNG & Signing Tests - Quick Command Reference
# Все необходимые команды для полного цикла тестирования

# ============================================================================
# УСТАНОВКА ЗАВИСИМОСТЕЙ
# ============================================================================

echo "=== Installing Dependencies ==="

# Python packages
pip install pyserial requests

# System packages
sudo apt update
sudo apt install doxygen dieharder

# ============================================================================
# ФАЗА 1: ПОДГОТОВКА К RNG ТЕСТИРОВАНИЮ
# ============================================================================

echo ""
echo "=== Phase 1: RNG Preparation ==="

cd /data/projects/CryptoWallet

# Сборка прошивки с RNG dump
make clean
make USE_CRYPTO_SIGN=1 USE_TEST_SEED=1 USE_RNG_DUMP=1 -j4

# Проверка размера
arm-none-eabi-size build/firmware.elf

# Прошивка на устройство
make flash

# Проверка интеграции trezor-crypto
arm-none-eabi-nm build/firmware.elf | grep -E "bip39|ecdsa|secp256k1"

# ============================================================================
# ФАЗА 2: ЗАХВАТ RNG ДАННЫХ (30 мин)
# ============================================================================

echo ""
echo "=== Phase 2: RNG Data Capture (30 minutes at 115200 baud) ==="

# Проверить UART вывод (должны видеть только сырые байты)
# screen /dev/ttyACM0 115200
# (Ctrl-A, Ctrl-\ для выхода)

# Захват 128 MiB RNG данных (стандартный размер для Dieharder)
python3 scripts/test_rng_signing_comprehensive.py --mode rng --port /dev/ttyACM0

# Проверка файла
ls -lh rng.bin
file rng.bin

# Если нужны 256 MiB (для более надёжного теста)
python3 scripts/test_rng_signing_comprehensive.py --mode rng --bytes 268435456 --output rng_256mb.bin

# ============================================================================
# ФАЗА 3: АНАЛИЗ КАЧЕСТВА RNG
# ============================================================================

echo ""
echo "=== Phase 3: RNG Quality Analysis ==="

# Быстрый анализ (entropy, chi-square, distribution)
python3 scripts/test_rng_signing_comprehensive.py --mode rng --file rng.bin

# Статистика файла
stat rng.bin

# SHA256 для верификации целостности
sha256sum rng.bin > rng.bin.sha256

# ============================================================================
# ФАЗА 4: DIEHARDER СТАТИСТИЧЕСКИЕ ТЕСТЫ (1-3 часа)
# ============================================================================

echo ""
echo "=== Phase 4: DIEHARDER Statistical Tests (1-3 hours) ==="

# Список доступных тестов
dieharder -l

# Все тесты (обычно 100+ тестов)
python3 scripts/test_rng_signing_comprehensive.py --mode dieharder --file rng.bin \
  | tee dieharder_results.txt

# Или отдельные тесты для отладки
dieharder -g 201 -f rng.bin -d 1          # Тест 1 (birthdays)
dieharder -g 201 -f rng.bin -d 2          # Тест 2
dieharder -g 201 -f rng.bin -d 1 -p 0     # Тест 1, subtest 0

# Анализ результатов
grep "PASS" dieharder_results.txt | wc -l
grep "FAIL" dieharder_results.txt | wc -l
grep "WEAK" dieharder_results.txt | wc -l

# Сохранить результаты
cp dieharder_results.txt dieharder_results_$(date +%Y%m%d_%H%M%S).txt

# ============================================================================
# ФАЗА 5: ПОДГОТОВКА К ТЕСТИРОВАНИЮ ПОДПИСАНИЯ
# ============================================================================

echo ""
echo "=== Phase 5: Transaction Signing Preparation ==="

# Пересборка прошивки с HTTP вместо RNG dump
make clean
make USE_CRYPTO_SIGN=1 USE_TEST_SEED=1 USE_LWIP=1 -j4

# Прошивка
make flash

# Проверка конфигурации
grep -n "USE_LWIP\|USE_CRYPTO_SIGN\|USE_TEST_SEED" Makefile | head -10

# ============================================================================
# ФАЗА 6: ПРОВЕРКА УСТРОЙСТВА
# ============================================================================

echo ""
echo "=== Phase 6: Device Connectivity Check ==="

# Дождаться включения DHCP (если используется)
sleep 5

# Пинг устройства
ping -c 3 192.168.0.10

# Проверить статус через curl
curl http://192.168.0.10/status
# Ожидаемый ответ: {"status":"ready"}

# Или через Python requests
python3 << 'EOF'
import requests
try:
    r = requests.get('http://192.168.0.10/status', timeout=5)
    print(f"Status: {r.status_code}")
    print(f"Response: {r.json()}")
except Exception as e:
    print(f"Error: {e}")
EOF

# ============================================================================
# ФАЗА 7: ТЕСТИРОВАНИЕ ПОДПИСАНИЯ ТРАНЗАКЦИЙ
# ============================================================================

echo ""
echo "=== Phase 7: Transaction Signing Tests ==="

# Запустить полный набор тестов подписания
python3 scripts/test_rng_signing_comprehensive.py --mode signing --ip 192.168.0.10

# Или на другом IP
python3 scripts/test_rng_signing_comprehensive.py --mode signing --ip 192.168.1.100

# Ручная отправка транзакции (для отладки)
python3 << 'EOF'
import requests
import json

tx = {
    "recipient": "1LqBGSKuX5yYUonjxT5qGfpUsXKYYWeabA",
    "amount": "0.5",
    "currency": "BTC"
}

# POST транзакция
r = requests.post("http://192.168.0.10/tx", json=tx, timeout=5)
print(f"POST: {r.status_code}")

# Ждём подтверждения (нужно нажать кнопку!)
import time
time.sleep(2)

# GET подпись
r = requests.get("http://192.168.0.10/tx/signed", timeout=5)
print(f"GET: {r.status_code}")
print(f"Response: {r.json()}")
EOF

# ============================================================================
# ФАЗА 8: ПРОВЕРКА ДЕТЕРМИНИЗМА (RFC6979)
# ============================================================================

echo ""
echo "=== Phase 8: RFC6979 Determinism Test ==="

# Проверить: одинаковые TX → одинаковые подписи
python3 scripts/test_rng_signing_comprehensive.py --mode signing --ip 192.168.0.10 \
  | grep -A 10 "Deterministic"

# ============================================================================
# ФАЗА 9: ПОЛНОЕ ТЕСТИРОВАНИЕ (ВСЕ СИСТЕМЫ)
# ============================================================================

echo ""
echo "=== Phase 9: Complete Verification (ALL) ==="

# Автоматически запустит: RNG capture + DIEHARDER + Signing tests
python3 scripts/test_rng_signing_comprehensive.py --mode verify-all \
  | tee full_test_results_$(date +%Y%m%d_%H%M%S).txt

# ============================================================================
# ДОПОЛНИТЕЛЬНЫЕ КОМАНДЫ ДЛЯ ОТЛАДКИ
# ============================================================================

echo ""
echo "=== Debug Commands ==="

# 1. Проверить наличие Doxygen комментариев
rg "@brief|@details" Core/Src/*.c | head -20

# 2. Регенерировать документацию
make docs-md
make docs-doxygen
make docs

# 3. Проверить конфигурацию trezor-crypto
arm-none-eabi-nm build/firmware.elf | grep random_buffer
arm-none-eabi-nm build/firmware.elf | grep memzero

# 4. Проверить размеры секций
arm-none-eabi-objdump -h build/firmware.elf | grep -E "\.text|\.rodata|\.data"

# 5. Список всех символов
arm-none-eabi-nm build/firmware.elf | sort | tail -50

# 6. Проверить LCG параметры
grep -n "1664525\|1013904223" Core/Src/crypto_wallet.c

# 7. Проверить использование memzero
grep -n "memzero" Core/Src/crypto_wallet.c

# 8. Проверить RFC6979
grep -n "rfc6979\|is_canonical" Core/Src/crypto_wallet.c

# ============================================================================
# ИТОГОВЫЕ КОМАНДЫ ОТЧЁТА
# ============================================================================

echo ""
echo "=== Generate Test Report ==="

# Создать финальный отчёт
cat > cryptowallet_test_report.md << REPORT
# CryptoWallet Security Test Report

**Date:** $(date)
**Firmware:** $(arm-none-eabi-size build/firmware.elf | tail -1)

## RNG Analysis
- File: $(ls -lh rng.bin | awk '{print $9, $5}')
- SHA256: $(sha256sum rng.bin | cut -d' ' -f1)
- Entropy: [заполнить из результатов]
- Chi-square: [заполнить из результатов]

## DIEHARDER Results
- PASS tests: $(grep -c "PASS" dieharder_results.txt 2>/dev/null || echo "?")
- FAIL tests: $(grep -c "FAIL" dieharder_results.txt 2>/dev/null || echo "?")
- WEAK tests: $(grep -c "WEAK" dieharder_results.txt 2>/dev/null || echo "?")

## Signing Verification
- Device IP: 192.168.0.10
- HTTP Status: $(curl -s http://192.168.0.10/status | jq . 2>/dev/null || echo "Offline")
- Test Results: [заполнить вручную]
- RFC6979 Determinism: [заполнить вручную]

## Recommendations
- [заполнить вручную]

REPORT

cat cryptowallet_test_report.md

# ============================================================================
# ОЧИСТКА И АРХИВИРОВАНИЕ
# ============================================================================

echo ""
echo "=== Cleanup and Archive ==="

# Архивировать результаты
mkdir -p test_results_$(date +%Y%m%d)
cp rng.bin dieharder_results.txt cryptowallet_test_report.md test_results_$(date +%Y%m%d)/

# Сжать архив
tar -czf cryptowallet_tests_$(date +%Y%m%d).tar.gz test_results_$(date +%Y%m%d)/

# Вывести информацию об архиве
ls -lh cryptowallet_tests_*.tar.gz

# ============================================================================
# УСПЕШНОЕ ЗАВЕРШЕНИЕ
# ============================================================================

echo ""
echo "✓ All tests completed successfully!"
echo ""
echo "Results:"
echo "  - rng.bin (128 MiB binary RNG data)"
echo "  - dieharder_results.txt (statistical test output)"
echo "  - cryptowallet_test_report.md (summary report)"
echo "  - test_results_YYYYMMDD/ (archived results)"
echo ""
echo "Next steps:"
echo "  1. Review dieharder_results.txt for failures"
echo "  2. Verify RFC6979 determinism in test output"
echo "  3. Archive results for compliance documentation"
echo ""
