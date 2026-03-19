/**
  ******************************************************************************
  * @file    wallet_seed.c
  * @brief   Strong get_wallet_seed() when USE_TEST_SEED=1 (development only).
  ******************************************************************************
  * @details
  *          BIP-39 vector "abandon ... about" → 64-byte seed. **Never for real funds.**
  *          Production: replace with secure element / flash encryption workflow.
  *          **Audit:** scripts/bootloader_secure_signing_test.py --elf-audit-only .
  *          **Build:** Makefile USE_TEST_SEED=1 (implies USE_CRYPTO_SIGN).
  ******************************************************************************
  */

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "memzero.h"

#if defined(USE_CRYPTO_SIGN) && (USE_CRYPTO_SIGN == 1) && defined(USE_TEST_SEED) && (USE_TEST_SEED == 1)
#include "bip39.h"

/**
 * @brief Provide wallet seed from test mnemonic (dev only).
 * @note BIP-39 test vector. First address: 1LqBGSKuX5yYUonjxT5qGfpUsXKYYWeabA
 */
int get_wallet_seed(uint8_t *seed_out, size_t max_len)
{
    if (seed_out == NULL || max_len < 64) return -1;

    const char *mnemonic = "abandon abandon abandon abandon abandon abandon "
                          "abandon abandon abandon abandon abandon about";

    uint8_t seed[64];
    mnemonic_to_seed(mnemonic, "", seed, NULL);
    memcpy(seed_out, seed, 64);
    memzero(seed, sizeof(seed));
    return 0;
}
#else
/* Stub when USE_TEST_SEED not set — weak symbol in task_sign.c will be used */
#endif
