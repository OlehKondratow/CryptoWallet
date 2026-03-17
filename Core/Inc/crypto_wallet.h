/**
  ******************************************************************************
  * @file    crypto_wallet.h
  * @brief   trezor-crypto integration: RNG, BIP-39, BIP-32, ECDSA secp256k1.
  ******************************************************************************
  * @details Requires trezor-crypto (github.com/trezor/trezor-crypto).
  *          Target: STM32H743 (TRNG + ADC entropy). ARM GCC compatible.
  ******************************************************************************
  */

#ifndef __CRYPTO_WALLET_H__
#define __CRYPTO_WALLET_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

/*-----------------------------------------------------------------------------
 * Constants
 *-----------------------------------------------------------------------------*/
#define CRYPTO_ENTROPY_128_BITS    16U   /**< 128 bits = 12 words BIP-39 */
#define CRYPTO_MNEMONIC_MAX_LEN    256U  /**< Max mnemonic string length */
#define CRYPTO_SHA256_DIGEST_LEN   32U
#define CRYPTO_ECDSA_SIG_LEN       64U   /**< Compact signature (r||s) */

/*-----------------------------------------------------------------------------
 * RNG initialization (STM32 TRNG + ADC entropy)
 *-----------------------------------------------------------------------------*/

/**
 * @brief Initialize RNG for trezor-crypto random_buffer.
 * @details Enables STM32 RNG peripheral and seeds with TRNG + ADC noise.
 *          Call before any BIP-39/BIP-32 operation.
 * @return 0 on success, -1 on error.
 */
int crypto_rng_init(void);

/*-----------------------------------------------------------------------------
 * BIP-39 (Mnemonic)
 *-----------------------------------------------------------------------------*/

/**
 * @brief Generate 12-word mnemonic from 128 bits entropy.
 * @param entropy 16 bytes (128 bits) of entropy.
 * @param mnemonic_out Output buffer for mnemonic string.
 * @param mnemonic_size Size of mnemonic_out (min CRYPTO_MNEMONIC_MAX_LEN).
 * @return 0 on success, -1 on error.
 * @note Uses mnemonic_from_data from trezor-crypto/bip39.c
 *       Include: crypto/bip39.h
 */
int crypto_entropy_to_mnemonic_12(const uint8_t entropy[CRYPTO_ENTROPY_128_BITS],
                                  char *mnemonic_out, size_t mnemonic_size);

/*-----------------------------------------------------------------------------
 * BIP-32 (HD derivation)
 *-----------------------------------------------------------------------------*/

/**
 * @brief Derive Bitcoin key for path m/44'/0'/0'/0/0 from seed.
 * @param seed BIP-39 seed (64 bytes from mnemonic_to_seed).
 * @param seed_len Seed length (typically 64).
 * @param priv_key_out Output: 32-byte private key.
 * @return 0 on success, -1 on error.
 * @note Uses hdnode_from_seed, hdnode_private_ckd from trezor-crypto/bip32.c
 *       Include: crypto/bip32.h, crypto/secp256k1.h
 */
int crypto_derive_btc_m44_0_0_0_0(const uint8_t *seed, size_t seed_len,
                                  uint8_t priv_key_out[32]);

/*-----------------------------------------------------------------------------
 * ECDSA signature (secp256k1)
 *-----------------------------------------------------------------------------*/

/**
 * @brief Sign 32-byte hash with secp256k1, compact format (64 bytes).
 * @param priv_key 32-byte private key.
 * @param hash 32-byte message hash (e.g. SHA-256 of tx).
 * @param sig_out Output: 64-byte compact signature (r||s).
 * @return 0 on success, -1 on error.
 * @note Uses ecdsa_sign_digest from trezor-crypto/ecdsa.c
 *       Include: crypto/ecdsa.h, crypto/secp256k1.h
 *       priv_key is memzero'd after use — pass writable buffer.
 */
int crypto_sign_btc_hash(uint8_t priv_key[32],
                        const uint8_t hash[CRYPTO_SHA256_DIGEST_LEN],
                        uint8_t sig_out[CRYPTO_ECDSA_SIG_LEN]);

#ifdef __cplusplus
}
#endif

#endif /* __CRYPTO_WALLET_H__ */
