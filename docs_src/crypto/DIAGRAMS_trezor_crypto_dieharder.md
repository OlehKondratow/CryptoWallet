# Архитектурная диаграмма: trezor-crypto + Dieharder в CryptoWallet

## 1. Слои интеграции trezor-crypto

```
┌─────────────────────────────────────────────────────────────┐
│             ПРИЛОЖЕНИЕ (Application Layer)                 │
│  ┌──────────────┐  ┌─────────────┐  ┌───────────────┐      │
│  │  task_net    │  │  task_sign  │  │  task_display │      │
│  │ (HTTP /tx)   │  │ (FSM sign)  │  │ (OLED UI)     │      │
│  └──────────────┘  └─────────────┘  └───────────────┘      │
│         ↑                  ↓                                 │
│    Queue g_tx_queue  Event g_user_event_group              │
└─────────────────────────────────────────────────────────────┘
                           ↓
┌─────────────────────────────────────────────────────────────┐
│        АДАПТЕР СЛОЙ (Adapter - crypto_wallet.c)            │
│                                                              │
│  crypto_wallet.h PUBLIC API:                               │
│  • crypto_rng_init()                                        │
│  • crypto_hash_sha256(data, len, digest)                   │
│  • crypto_entropy_to_mnemonic_12(entropy, mnemonic, size)  │
│  • crypto_derive_btc_m44_0_0_0_0(seed, len, privkey)       │
│  • crypto_sign_btc_hash(privkey, hash, sig)                │
└─────────────────────────────────────────────────────────────┘
                           ↓
┌─────────────────────────────────────────────────────────────┐
│     TREZOR-CRYPTO СЛОЙ (ThirdParty/trezor-crypto/)        │
│                                                              │
│  BIP-39           BIP-32             ECDSA                 │
│  ┌──────────┐  ┌──────────────┐  ┌──────────────┐          │
│  │ bip39.c  │  │  bip32.c     │  │  ecdsa.c     │          │
│  │ • mnem.  │  │ • HDNode     │  │ • sign       │          │
│  │ • from   │  │ • CKD prime  │  │ • verify     │          │
│  │  data    │  │ • CKD        │  │ • RFC6979    │          │
│  └──────────┘  └──────────────┘  └──────────────┘          │
│                                                              │
│  SHA-256          secp256k1         Вспомогат.            │
│  ┌──────────┐  ┌──────────────┐  ┌──────────────┐          │
│  │ sha2.c   │  │ secp256k1.c  │  │ base58.c     │          │
│  │ • Raw    │  │ • Curve      │  │ • decode_chk │          │
│  │ • Init   │  │   params     │  │              │          │
│  │ • Update │  │              │  │ hmac.c       │          │
│  │ • Finish │  │              │  │ pbkdf2.c     │          │
│  └──────────┘  └──────────────┘  └──────────────┘          │
│                                                              │
│  RNG Hook:  random_buffer(), random32(), random_reseed()  │
└─────────────────────────────────────────────────────────────┘
                           ↓
┌─────────────────────────────────────────────────────────────┐
│              STM32H7 HAL СЛОЙ (Hardware)                    │
│                                                              │
│  ┌──────────────────┐         ┌──────────────────┐          │
│  │  HAL_RNG         │         │  HAL_GetTick()   │          │
│  │  GenerateRandom  │         │  (Timer entropy) │          │
│  │  (TRNG: 32 бит)  │         │                  │          │
│  └──────────────────┘         └──────────────────┘          │
└─────────────────────────────────────────────────────────────┘
```

---

## 2. Поток данных: HTTP POST → Подпись

```
╔═══════════════════════════════════════════════════════════════════════╗
║                                                                       ║
║  HTTP CLIENT (PC)                                                     ║
║  POST http://192.168.0.10/tx                                         ║
║  Content-Type: application/json                                      ║
║  {                                                                    ║
║    "recipient": "1LqBGSKuX5yYUonjxT5qGfpUsXKYYWeabA",             ║
║    "amount": "0.5",                                                   ║
║    "currency": "BTC"                                                  ║
║  }                                                                    ║
║                                                                       ║
╚═══════════════════════════════════════════════════════════════════════╝
                                ↓
                         Network: Ethernet
                                ↓
╔═══════════════════════════════════════════════════════════════════════╗
║  [1] task_net.c — HTTP Parser                                        ║
║  ──────────────────────────────────────────                           ║
║  Status: WAITING_TX → RECEIVED                                       ║
║  Action: xQueueSend(g_tx_queue, &tx_request, ...)                   ║
║  Output: wallet_tx_t {                                               ║
║    recipient = "1LqBG...",                                          ║
║    amount = "0.5",                                                    ║
║    currency = "BTC",                                                  ║
║    status = TX_PENDING                                                ║
║  }                                                                    ║
╚═══════════════════════════════════════════════════════════════════════╝
                                ↓
                         g_tx_queue (IPC)
                                ↓
╔═══════════════════════════════════════════════════════════════════════╗
║  [2] task_sign.c — Receive & Validate                               ║
║  ────────────────────────────────────────                             ║
║  xQueueReceive(g_tx_queue, &tx, pdMS_TO_TICKS(100))                 ║
║                                                                       ║
║  ┌────────────────────────────────────────────┐                      ║
║  │ tx_request_validate.c                      │                      ║
║  │ • base58_decode_check(recipient)  [trezor] │                      ║
║  │ • regex_match(amount, "^[0-9.]+$")         │                      ║
║  │ • whitelist_check(currency)                │                      ║
║  │ Result: VALID or INVALID                   │                      ║
║  └────────────────────────────────────────────┘                      ║
║                                                                       ║
║  State: SIGN_RECEIVED                                                │
╚═══════════════════════════════════════════════════════════════════════╝
                                ↓
╔═══════════════════════════════════════════════════════════════════════╗
║  [3] task_sign.c — Build Hash Input                                 ║
║  ──────────────────────────────────────                               ║
║  build_hash_input(tx, buf, 256)                                     ║
║                                                                       ║
║  Format: "recipient|amount|currency"                                │
║  Result: "1LqBGSK...|0.5|BTC"  (NUL-terminated)                     ║
╚═══════════════════════════════════════════════════════════════════════╝
                                ↓
╔═══════════════════════════════════════════════════════════════════════╗
║  [4] crypto_wallet.c → SHA-256                                       ║
║  ──────────────────────────────────────────                           ║
║  crypto_hash_sha256(data, len, digest_out)                          ║
║                                                                       ║
║  #if USE_CRYPTO_SIGN == 1                                            ║
║    sha256_Raw(data, len, digest_out)  [trezor-crypto/sha2.c]       ║
║  #else                                                                ║
║    sha256_minimal(data, len, digest_out)  [minimal implementation]  ║
║  #endif                                                               ║
║                                                                       ║
║  Output: 32-byte digest                                              ║
║  digest = { 0xAB, 0xCD, ..., 0xEF }  (шестнадцатеричный)           ║
╚═══════════════════════════════════════════════════════════════════════╝
                                ↓
╔═══════════════════════════════════════════════════════════════════════╗
║  [5] task_display.c — Display Transaction                           ║
║  ────────────────────────────────────────                             ║
║  Update OLED (128x32):                                               ║
║                                                                       ║
║  ┌──────────────────────────────────────┐                            ║
║  │  CONFIRM SIGNATURE?  ✓ ✗             │  ← User Button (PC13)     ║
║  │  1LqBGSK...                          │                            ║
║  │  0.5 BTC → Confirm (30s timeout)     │                            ║
║  └──────────────────────────────────────┘                            ║
║                                                                       ║
║  State: SIGN_WAIT_CONFIRM                                            ║
║  Timeout: 30 seconds (if no user action)                             ║
╚═══════════════════════════════════════════════════════════════════════╝
                                ↓
║  xEventGroupWaitBits(g_user_event_group,
║    EVENT_USER_CONFIRMED | EVENT_USER_REJECTED, ...)
                                │
                    ┌───────────┴───────────┐
                    ↓                       ↓
           ✓ CONFIRMED            ✗ REJECTED / TIMEOUT
                    ↓                       ↓
╔═══════════════════════════════════════════════════════════════════════╗
║  [6] task_sign.c — Get Wallet Seed                                  ║
║  ────────────────────────────────────────                             ║
║  get_wallet_seed(seed_buf, 64)                                       ║
║                                                                       ║
║  #if USE_TEST_SEED == 1                                              ║
║    seed = hardcoded BIP-39 vector                                    ║
║          (m. = "abandon abandon ... about" → first address)         ║
║  #else                                                                ║
║    seed = from secure storage (NOT YET IMPLEMENTED)                 ║
║  #endif                                                               ║
║                                                                       ║
║  Output: 64-byte seed (never logged, never transmitted)              ║
╚═══════════════════════════════════════════════════════════════════════╝
                                ↓
╔═══════════════════════════════════════════════════════════════════════╗
║  [7] crypto_wallet.c → BIP-32 Key Derivation                        ║
║  ──────────────────────────────────────────────                       ║
║  crypto_derive_btc_m44_0_0_0_0(seed, 64, priv_key_out)              ║
║                                                                       ║
║  trezor-crypto/bip32.c:                                              ║
║    HDNode node;                                                       ║
║    hdnode_from_seed(seed, 64, "secp256k1", &node)                   ║
║    hdnode_private_ckd_prime(&node, 44)  // m/44'                    ║
║    hdnode_private_ckd_prime(&node, 0)   // m/44'/0'                 ║
║    hdnode_private_ckd_prime(&node, 0)   // m/44'/0'/0'              ║
║    hdnode_private_ckd(&node, 0)         // m/44'/0'/0'/0            ║
║    hdnode_private_ckd(&node, 0)         // m/44'/0'/0'/0/0          ║
║    memcpy(priv_key_out, node.private_key, 32)                       ║
║    memzero(&node, sizeof(node))         [ОЧИСТКА!]                  ║
║                                                                       ║
║  Output: 32-byte private key (scalar)                                ║
╚═══════════════════════════════════════════════════════════════════════╝
                                ↓
╔═══════════════════════════════════════════════════════════════════════╗
║  [8] crypto_wallet.c → ECDSA Sign                                    ║
║  ──────────────────────────────────────                               ║
║  crypto_sign_btc_hash(priv_key, digest, sig_out)                    ║
║                                                                       ║
║  trezor-crypto/ecdsa.c:                                              ║
║    ecdsa_sign_digest(                                                ║
║      &secp256k1,       // Curve: Bitcoin standard                    ║
║      priv_key,         // 32-byte private scalar                     ║
║      hash,             // 32-byte SHA-256 digest                     ║
║      sig_out,          // Output: 64 bytes (r || s)                  ║
║      &pby,             // Recovery flag (not used)                   ║
║      is_canonical      // Callback (accept all for compact)         ║
║    )                                                                  ║
║                                                                       ║
║  RFC6979 (used internally in ecdsa_sign_digest):                    ║
║    k = deterministic_random_k(priv_key, digest)                     ║
║    [SAME hash + SAME key → SAME k → SAME signature]                ║
║                                                                       ║
║  memzero(priv_key, 32)  [ОЧИСТКА ПРИВАТНОГО КЛЮЧА!]                ║
║                                                                       ║
║  Output: 64-byte signature (r || s, 32 bytes each, big-endian)     ║
╚═══════════════════════════════════════════════════════════════════════╝
                                ↓
╔═══════════════════════════════════════════════════════════════════════╗
║  [9] Response to Client                                             ║
║  ──────────────────────────                                          ║
║  HTTP GET /tx/signed                                                │
║                                                                       ║
║  Response:                                                           ║
║  {                                                                   ║
║    "status": "signed",                                               ║
║    "signature": "ABCDEF123...XYZ",  [128 hex chars = 64 bytes]     ║
║    "timestamp": 1234567890                                          ║
║  }                                                                    ║
║                                                                       ║
║  Or WebUSB Endpoint 2 (device → host):                              ║
║    Raw 64-byte binary signature                                      ║
╚═══════════════════════════════════════════════════════════════════════╝
```

---

## 3. RNG поток: TRNG + LCG → Dieharder

```
┌────────────────────────────────────────────────────────────────────┐
│  STM32H743 Hardware RNG (TRNG)                                     │
│                                                                    │
│  ┌──────────────────┐        ┌──────────────────┐                │
│  │ Oscillator 1     │        │ Sample Register  │                │
│  │ ~ 48 MHz         │───────→│ (32-bit word)    │                │
│  └──────────────────┘        └──────────────────┘                │
│                                       ↓                            │
│                        HAL_RNG_GenerateRandomNumber()             │
│                        [1 word/poll, ~2.7 µs]                     │
└────────────────────────────────────────────────────────────────────┘
                                       ↓
┌────────────────────────────────────────────────────────────────────┐
│  Software Entropy Pool (LCG)                                       │
│                                                                    │
│  s_rng_entropy_pool = 32-bit state                                │
│  Multiplier: 1664525 (Numerical Recipes)                          │
│  Increment:  1013904223                                           │
│                                                                    │
│  Update: s_pool = (s_pool * 1664525) + 1013904223               │
│          [Linear Congruential Generator]                           │
└────────────────────────────────────────────────────────────────────┘
                                       ↓
┌────────────────────────────────────────────────────────────────────┐
│  crypto_wallet.c: random_buffer(buf, len)                         │
│                                                                    │
│  FOR EACH 4-byte chunk:                                           │
│    ┌─────────────────────────────────────────┐                    │
│    │ IF (TRNG available)                     │                    │
│    │   rng_val = HAL_RNG_GenerateRandomNumber()                  │
│    │   rng_val ^= s_rng_entropy_pool         [XOR mixing]        │
│    │   s_rng_entropy_pool = (s_pool * 1664525) + 1013904223      │
│    │ ELSE                                    │                    │
│    │   rng_val = s_rng_entropy_pool          [fallback]          │
│    │   s_rng_entropy_pool = (s_pool * 1664525) + 1013904223      │
│    │ END                                     │                    │
│    │                                         │                    │
│    │ memcpy(buf[i], rng_val, 4)             [copy to output]    │
│    └─────────────────────────────────────────┘                    │
└────────────────────────────────────────────────────────────────────┘
                                       ↓
            ┌─────────────────────────────────────────┐
            │ random_buffer()                         │
            │ ↑ Called by trezor-crypto                │
            │ • mnemonic_from_data()                   │
            │ • hdnode_private_ckd()                   │
            │ • ecdsa_sign_digest() [RFC6979 k]        │
            │ • Other RNG operations                   │
            └─────────────────────────────────────────┘
                                       ↓
                    ┌──────────────────────────────┐
                    │ Cryptographic Operations    │
                    │ (all use random_buffer RNG) │
                    └──────────────────────────────┘
```

---

## 4. Dieharder Testing Pipeline

```
┌────────────────────────────────────────────────────────────────────┐
│ ЭТАП 1: Подготовка Прошивки                                       │
│                                                                    │
│ $ make clean                                                       │
│ $ make USE_CRYPTO_SIGN=1 USE_TEST_SEED=1 USE_RNG_DUMP=1           │
│ $ make flash                                                       │
│                                                                    │
│ #define USE_RNG_DUMP 1                                            │
│ → Firmware outputs ONLY binary RNG bytes on UART                  │
│ → NO printf, NO text, NO logging                                  │
└────────────────────────────────────────────────────────────────────┘
                                       ↓
┌────────────────────────────────────────────────────────────────────┐
│ ЭТАП 2: Запуск на девайсе (Terminal 1)                           │
│                                                                    │
│ [Device connected to /dev/ttyACM0]                                │
│ STM32H743 boots → calls crypto_rng_init()                        │
│ Loop: random_buffer() → UART TX → raw bytes                       │
│                                                                    │
│ (No visible output on serial monitor)                             │
└────────────────────────────────────────────────────────────────────┘
                                       ↓
┌────────────────────────────────────────────────────────────────────┐
│ ЭТАП 3: Захват потока (Terminal 2)                               │
│                                                                    │
│ $ python3 scripts/capture_rng_uart.py \                           │
│     --port /dev/ttyACM0 \                                         │
│     --out rng.bin \                                                │
│     --bytes 134217728                                             │
│                                                                    │
│ [Прогресс каждые 8 MiB]                                          │
│   8 MiB                                                           │
│  16 MiB                                                           │
│  24 MiB                                                           │
│  ...                                                              │
│ 128 MiB                                                           │
│ Done: 134217728 bytes written.                                    │
│                                                                    │
│ [Время: ~30 минут @ 115200 baud]                                 │
└────────────────────────────────────────────────────────────────────┘
                                       ↓
│ ls -lh rng.bin
│ -rw-r--r-- 1 user user 128M Mar 19 14:30 rng.bin
                                       ↓
┌────────────────────────────────────────────────────────────────────┐
│ ЭТАП 4: Установка Dieharder (один раз)                           │
│                                                                    │
│ $ sudo apt install dieharder                                      │
│ $ dieharder -l  (проверка)                                        │
│ test_num = 0   diehard_birthdays                                  │
│ test_num = 1   diehard_operm5                                     │
│ ...                                                               │
└────────────────────────────────────────────────────────────────────┘
                                       ↓
┌────────────────────────────────────────────────────────────────────┐
│ ЭТАП 5: Запуск тестов                                             │
│                                                                    │
│ $ python3 scripts/run_dieharder.py --file rng.bin                 │
│                                                                    │
│ [Внутри: dieharder -g 201 -f rng.bin -a]                         │
│          201 = file_input_raw (binary stream)                     │
│          -a = all tests                                           │
│                                                                    │
│ [Выполнение: 30 мин — несколько часов]                           │
│                                                                    │
│ Output:                                                           │
│ diehard_birthdays                                                 │
│   p-value = 0.523  PASS ✓                                        │
│ diehard_operm5                                                    │
│   p-value = 0.127  PASS ✓                                        │
│ diehard_rank_32x32                                                │
│   p-value = 0.001  FAIL ✗  (смещение!)                          │
│ ...                                                               │
└────────────────────────────────────────────────────────────────────┘
                                       ↓
┌────────────────────────────────────────────────────────────────────┐
│ ЭТАП 6: Анализ результатов                                        │
│                                                                    │
│ ✓ PASS (0.001 < p < 0.999)       OK                             │
│ ⚠ WEAK (0.0001 < p < 0.001)      Re-test with larger file       │
│ ✗ FAIL (p < 0.0001)              RNG defect, investigate         │
│ ✗ FAIL (p > 0.9999)              Anti-correlation, check LCG     │
│                                                                    │
│ Typical output:                                                   │
│ • 80-95% PASS    → RNG is acceptable                             │
│ • 5-15% WEAK     → Borderline, retest with 256 MiB               │
│ • >5% FAIL       → Serious issues, debug entropy sources         │
│                                                                    │
│ Archive results: tee dieharder_results.txt                        │
└────────────────────────────────────────────────────────────────────┘
```

---

## 5. Точки интеграции: Где trezor-crypto вызывается

```
task_sign.c (main signing FSM)
    │
    ├─→ tx_request_validate.c
    │       └─→ base58_decode_check()         [trezor-crypto/base58.c]
    │
    ├─→ build_hash_input()
    │       └─→ sprintf()
    │
    ├─→ crypto_wallet.h API
    │       ├─→ crypto_hash_sha256()
    │       │       └─→ sha256_Raw()          [trezor-crypto/sha2.c]
    │       │
    │       ├─→ crypto_derive_btc_m44_0_0_0_0()
    │       │       ├─→ hdnode_from_seed()    [trezor-crypto/bip32.c]
    │       │       ├─→ hdnode_private_ckd_prime() × 3
    │       │       ├─→ hdnode_private_ckd() × 2
    │       │       └─→ memzero()             [Core/Src/memzero.c]
    │       │
    │       └─→ crypto_sign_btc_hash()
    │               ├─→ ecdsa_sign_digest()   [trezor-crypto/ecdsa.c]
    │               │       ├─→ rfc6979_generate_k()  [internal]
    │               │        └─→ random_buffer()      [trezor interface]
    │               └─→ memzero()             [очистка privkey]
    │
    └─→ Send response (g_last_sig)

random_buffer()  [crypto_wallet.c]
    │
    ├─→ HAL_RNG_GenerateRandomNumber()   [STM32 HAL]
    │
    └─→ LCG entropy pool update
```

---

## 6. Компиляционные флаги и их влияние

```
┌──────────────────────────────────────────────────────────────────┐
│ make USE_CRYPTO_SIGN=1 USE_TEST_SEED=1 USE_RNG_DUMP=1            │
└──────────────────────────────────────────────────────────────────┘

Флаг                 Значение    Эффект
─────────────────────────────────────────────────────────────────────

USE_CRYPTO_SIGN      1          • Линкуются bip39, bip32, ecdsa, secp256k1
                                 • sha2, rand, hmac, pbkdf2, rfc6979
                                 • Размер: ~100-150 KB
                                 • Полная функциональность

                    0          • Только sha2_minimal
                                 • Размер: ~5 KB
                                 • Только SHA-256

USE_TEST_SEED        1          • get_wallet_seed() → hardcoded BIP-39
                                 • m. = "abandon abandon ... about"
                                 • Bitcoin addr: 1LqBGSKuX5yYUonjxT5qGfpUsXKYYWeabA
                                 • Детерминированные подписи
                                 • ⚠️ ТОЛЬКО для разработки!

                    0          • get_wallet_seed() → weak stub
                                 • Возвращает -1 (нет seed)
                                 • Подписание невозможно

USE_RNG_DUMP         1          • Выводит сырые байты random_buffer() на UART
                                 • БЕЗ текстового логирования
                                 • Для Dieharder тестирования
                                 • ⚠️ Отключает printf на UART

                    0          • Нормальное логирование
                                 • Нет RNG дампа

USE_LWIP             1          • Ethernet включен
                                 • HTTP /tx endpoint
                                 • DHCP + static IP fallback

                    0          • Только WebUSB/UART
                                 • Меньше памяти

USE_WEBUSB           1          • WebUSB endpoint
                                 • USB-HS interface
                                 • Альтернатива Ethernet

                    0          • Только Ethernet/UART

BOOT_TEST            1          • Диагностический режим
                                 • БЕЗ FreeRTOS
                                 • UART debug loop
```

---

**Конец диаграмм**
