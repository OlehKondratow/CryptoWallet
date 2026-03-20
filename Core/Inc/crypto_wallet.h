/**
  ******************************************************************************
  * @file    crypto_wallet.h
  * @brief   trezor-crypto integration: RNG, BIP-39, BIP-32, ECDSA secp256k1.
  ******************************************************************************
 * @details Requires trezor-crypto (github.com/trezor/trezor-crypto).
 *          Target: STM32H743 (TRNG + timer entropy). ARM GCC compatible.
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
 * RNG initialization (STM32 TRNG + timer entropy)
 *-----------------------------------------------------------------------------*/

/**
 * @brief   Start STM32 RNG and entropy plumbing for trezor-crypto.
 * @details Configures @c HAL_RNG / peripheral as required by the port; mixes timer and
 *          optional entropy into @c random_buffer() path. Call once from @c main.c before
 *          @c Task_Sign_Create or any crypto that reads randomness. Idempotent-safe only if HAL allows.
 * @return  @c 0 on success, @c -1 if HAL RNG init fails.
 */
int crypto_rng_init(void);

/**
 * @brief   Generate 32-bit random value
 * @details Uses mixed TRNG and entropy pool.
 * @return  Random 32-bit value
 */
uint32_t random32(void);

/*-----------------------------------------------------------------------------
 * BIP-39 (Mnemonic)
 *-----------------------------------------------------------------------------*/

/**
 * @brief   Build a 12-word BIP-39 English mnemonic from 128-bit entropy.
 * @details Wraps trezor-crypto @c mnemonic_from_data . Output is space-separated words with
 *          no trailing newline. Caller must supply cryptographically strong @a entropy when used in production.
 * @param   entropy        Exactly @c CRYPTO_ENTROPY_128_BITS bytes.
 * @param   mnemonic_out   NUL-terminated string buffer.
 * @param   mnemonic_size  Capacity of @a mnemonic_out (>= @c CRYPTO_MNEMONIC_MAX_LEN recommended).
 * @return  @c 0 on success, @c -1 on internal error or buffer too small.
 * @note    Requires @c USE_CRYPTO_SIGN and trezor @c bip39.c in the link.
 */
int crypto_entropy_to_mnemonic_12(const uint8_t entropy[CRYPTO_ENTROPY_128_BITS],
                                  char *mnemonic_out, size_t mnemonic_size);

/*-----------------------------------------------------------------------------
 * BIP-32 (HD derivation)
 *-----------------------------------------------------------------------------*/

/**
 * @brief   Derive secp256k1 private key at hardened path @c m/44'/0'/0'/0/0 .
 * @details Standard BIP-44 account 0, external chain, first address. Uses trezor HDNode APIs.
 *          @a priv_key_out is overwritten on success; caller should @c memzero when done.
 * @param   seed           BIP-39 seed bytes (typically 64 from @c mnemonic_to_seed ).
 * @param   seed_len       Length of @a seed (usually 64).
 * @param   priv_key_out   32-byte big-endian private scalar output.
 * @return  @c 0 on success, @c -1 on derivation failure.
 */
int crypto_derive_btc_m44_0_0_0_0(const uint8_t *seed, size_t seed_len,
                                  uint8_t priv_key_out[32]);

/*-----------------------------------------------------------------------------
 * SHA-256 hash
 *-----------------------------------------------------------------------------*/

/**
 * @brief   SHA-256 over an arbitrary byte string.
 * @details With @c USE_CRYPTO_SIGN uses trezor @c sha256_Raw ; otherwise falls back to
 *          @c sha256_minimal() in this project.
 * @param   data        Input (may be @c NULL only if @a len is 0).
 * @param   len         Number of bytes to hash.
 * @param   digest_out  32-byte digest output.
 * @return  @c 0 on success, @c -1 on error.
 */
int crypto_hash_sha256(const uint8_t *data, size_t len,
                       uint8_t digest_out[CRYPTO_SHA256_DIGEST_LEN]);

/*-----------------------------------------------------------------------------
 * ECDSA signature (secp256k1)
 *-----------------------------------------------------------------------------*/

/**
 * @brief   ECDSA sign @a hash with secp256k1, compact @c r||s output.
 * @details Calls trezor @c ecdsa_sign_digest (RFC6979 deterministic k when configured in library).
 *          **Security:** @a priv_key buffer is cleared by implementation after use; pass a writable array.
 * @param   priv_key  32-byte private key (big-endian scalar).
 * @param   hash      32-byte message digest (e.g. SHA-256 of canonical tx string).
 * @param   sig_out   64-byte compact signature (32-byte r, 32-byte s).
 * @return  @c 0 on success, @c -1 on failure.
 */
int crypto_sign_btc_hash(uint8_t priv_key[32],
                        const uint8_t hash[CRYPTO_SHA256_DIGEST_LEN],
                        uint8_t sig_out[CRYPTO_ECDSA_SIG_LEN]);

#ifdef __cplusplus
}
#endif

#endif /* __CRYPTO_WALLET_H__ */
