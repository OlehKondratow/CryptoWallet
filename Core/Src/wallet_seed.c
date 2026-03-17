/**
  ******************************************************************************
  * @file    wallet_seed.c
  * @brief   get_wallet_seed implementation — test mnemonic for dev only.
  ******************************************************************************
  * @details USE_TEST_SEED=1: uses BIP-39 test vector "abandon ... about".
  *          NEVER use for real funds. For production, implement secure storage.
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
