/**
  ******************************************************************************
  * @file    crypto_wallet.c
  * @brief   trezor-crypto integration: RNG, BIP-39, BIP-32, ECDSA secp256k1.
  ******************************************************************************
  * @details trezor-crypto files to include in build:
  *          crypto/bip39.c, crypto/bip32.c, crypto/ecdsa.c, crypto/secp256k1.c,
  *          crypto/sha2.c, crypto/hmac.c, crypto/pbkdf2.c, crypto/bignum.c,
  *          crypto/rand.c (or override random_buffer only)
  ******************************************************************************
  */

#include "crypto_wallet.h"
#include "memzero.h"
#include <string.h>

#if defined(USE_CRYPTO_SIGN) && (USE_CRYPTO_SIGN == 1)
/* trezor-crypto includes */
#include "bip39.h"
#include "bip32.h"
#include "ecdsa.h"
#include "secp256k1.h"
#include "rand.h"
#include "sha2.h"
#include "stm32h7xx_hal.h"
#else
#include "sha256_minimal.h"
#endif

#if defined(USE_CRYPTO_SIGN) && (USE_CRYPTO_SIGN == 1)
/*-----------------------------------------------------------------------------
 * RNG: STM32 TRNG + entropy mixing
 *-----------------------------------------------------------------------------*/
__attribute__((weak)) RNG_HandleTypeDef hrng = {0};

static volatile uint32_t s_rng_entropy_pool = 0U;

void random_reseed(const uint32_t value)
{
    s_rng_entropy_pool ^= value;
}

uint32_t random32(void)
{
    uint32_t r;
    random_buffer((uint8_t *)&r, sizeof(r));
    return r;
}

uint32_t random_uniform(uint32_t n)
{
    uint32_t x, max = 0xFFFFFFFFU - (0xFFFFFFFFU % n);
    while ((x = random32()) >= max) { (void)0; }
    return x / (max / n);
}

void random_permute(char *str, size_t len)
{
    for (int i = (int)len - 1; i >= 1; i--) {
        int j = (int)random_uniform((uint32_t)(i + 1));
        char t = str[j];
        str[j] = str[i];
        str[i] = t;
    }
}

void random_buffer(uint8_t *buf, size_t len)
{
    if (buf == NULL || len == 0U) return;

    for (size_t i = 0; i < len; i += 4) {
        uint32_t rng_val;
        if (hrng.Instance != NULL && HAL_RNG_GenerateRandomNumber(&hrng, &rng_val) == HAL_OK) {
            rng_val ^= s_rng_entropy_pool;
            s_rng_entropy_pool = (s_rng_entropy_pool * 1664525U) + 1013904223U;
        } else {
            rng_val = s_rng_entropy_pool;
            s_rng_entropy_pool = (s_rng_entropy_pool * 1664525U) + 1013904223U;
        }
        size_t n = len - i;
        if (n > 4) n = 4;
        memcpy(buf + i, &rng_val, n);
    }
}

int crypto_rng_init(void)
{
    if (hrng.Instance == NULL) return -1;

    uint32_t adc_noise = HAL_GetTick();
    for (volatile int i = 0; i < 8; i++) {
        adc_noise ^= (adc_noise << 13);
        adc_noise ^= (adc_noise >> 17);
        adc_noise ^= (adc_noise << 5);
    }
    random_reseed(adc_noise);

    return 0;
}
#else
int crypto_rng_init(void)
{
    return 0;
}
#endif

#if defined(USE_CRYPTO_SIGN) && (USE_CRYPTO_SIGN == 1)
/*-----------------------------------------------------------------------------
 * BIP-39: 128 bits -> 12 words
 *-----------------------------------------------------------------------------
 * trezor-crypto: bip39.c, bip39.h
 * mnemonic_from_data(data, len) — len=16 gives 12 words
 */

int crypto_entropy_to_mnemonic_12(const uint8_t entropy[CRYPTO_ENTROPY_128_BITS],
                                  char *mnemonic_out, size_t mnemonic_size)
{
    if (entropy == NULL || mnemonic_out == NULL || mnemonic_size < CRYPTO_MNEMONIC_MAX_LEN)
        return -1;

    const char *mnemonic = mnemonic_from_data(entropy, (int)CRYPTO_ENTROPY_128_BITS);
    if (mnemonic == NULL) return -1;

    (void)strncpy(mnemonic_out, mnemonic, mnemonic_size - 1);
    mnemonic_out[mnemonic_size - 1] = '\0';
    mnemonic_clear();  /* Clear internal mnemonic buffer */
    return 0;
}

/*-----------------------------------------------------------------------------
 * BIP-32: m/44'/0'/0'/0/0 (Bitcoin)
 *-----------------------------------------------------------------------------
 * trezor-crypto: bip32.c, bip32.h, secp256k1.c
 * Path: 44' (hardened), 0', 0', 0, 0
 * hdnode_private_ckd_prime = hdnode_private_ckd(node, i | 0x80000000)
 */

int crypto_derive_btc_m44_0_0_0_0(const uint8_t *seed, size_t seed_len,
                                  uint8_t priv_key_out[32])
{
    if (seed == NULL || priv_key_out == NULL) return -1;

    HDNode node;
    memzero(&node, sizeof(node));

    if (hdnode_from_seed(seed, (int)seed_len, "secp256k1", &node) != 0)
        return -1;

    /* m/44' */
    if (hdnode_private_ckd_prime(&node, 44) != 0) return -1;
    /* m/44'/0' */
    if (hdnode_private_ckd_prime(&node, 0) != 0) return -1;
    /* m/44'/0'/0' */
    if (hdnode_private_ckd_prime(&node, 0) != 0) return -1;
    /* m/44'/0'/0'/0 */
    if (hdnode_private_ckd(&node, 0) != 0) return -1;
    /* m/44'/0'/0'/0/0 */
    if (hdnode_private_ckd(&node, 0) != 0) return -1;

    memcpy(priv_key_out, node.private_key, 32);
    memzero(&node, sizeof(node));  /* Clear HDNode with private key */
    return 0;
}

/*-----------------------------------------------------------------------------
 * ECDSA: sign 32-byte hash with secp256k1
 *-----------------------------------------------------------------------------
 * trezor-crypto: ecdsa.c, ecdsa.h, secp256k1.c
 * ecdsa_sign_digest(curve, priv_key, digest, sig, pby, is_canonical)
 * Compact sig: 64 bytes (r || s), no DER.
 */

static int is_canonical(uint8_t by, uint8_t sig[64])
{
    (void)by;
    (void)sig;
    return 1;  /* Accept all for compact format */
}

int crypto_sign_btc_hash(uint8_t priv_key[32],
                         const uint8_t hash[CRYPTO_SHA256_DIGEST_LEN],
                         uint8_t sig_out[CRYPTO_ECDSA_SIG_LEN])
{
    if (priv_key == NULL || hash == NULL || sig_out == NULL) return -1;

    uint8_t pby = 0;
    int ret = ecdsa_sign_digest(&secp256k1, priv_key, hash, sig_out, &pby, is_canonical);

    /* Zero out private key immediately after use */
    memzero(priv_key, 32);

    return (ret == 0) ? 0 : -1;
}

#else
/* Stubs when USE_CRYPTO_SIGN=0 */
int crypto_entropy_to_mnemonic_12(const uint8_t entropy[CRYPTO_ENTROPY_128_BITS],
                                  char *mnemonic_out, size_t mnemonic_size)
{
    (void)entropy;
    (void)mnemonic_out;
    (void)mnemonic_size;
    return -1;
}

int crypto_derive_btc_m44_0_0_0_0(const uint8_t *seed, size_t seed_len,
                                  uint8_t priv_key_out[32])
{
    (void)seed;
    (void)seed_len;
    (void)priv_key_out;
    return -1;
}

int crypto_sign_btc_hash(uint8_t priv_key[32],
                         const uint8_t hash[CRYPTO_SHA256_DIGEST_LEN],
                         uint8_t sig_out[CRYPTO_ECDSA_SIG_LEN])
{
    (void)priv_key;
    (void)hash;
    (void)sig_out;
    return -1;
}
#endif

/*-----------------------------------------------------------------------------
 * SHA-256: one-shot hash (always compiled)
 *-----------------------------------------------------------------------------*/
int crypto_hash_sha256(const uint8_t *data, size_t len,
                       uint8_t digest_out[CRYPTO_SHA256_DIGEST_LEN])
{
    if (data == NULL || digest_out == NULL) return -1;
#if defined(USE_CRYPTO_SIGN) && (USE_CRYPTO_SIGN == 1)
    sha256_Raw(data, len, digest_out);
#else
    sha256_minimal(data, len, digest_out);
#endif
    return 0;
}
