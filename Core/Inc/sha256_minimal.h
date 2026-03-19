/**
 * @file    sha256_minimal.h
 * @brief   Public API for minimal SHA-256 when trezor-crypto is disabled.
 * @details Implemented in @c sha256_minimal.c . Output is big-endian digest (32 bytes).
 */

#ifndef __SHA256_MINIMAL_H__
#define __SHA256_MINIMAL_H__

#include <stdint.h>
#include <stddef.h>

/**
 * @brief   Compute SHA-256 hash of @a data .
 * @param   data    Input bytes; read-only.
 * @param   len     Input length; @c 0 produces the standard empty-string hash.
 * @param   digest  Output 32-byte buffer (caller-owned).
 */
void sha256_minimal(const uint8_t *data, size_t len, uint8_t digest[32]);

#endif
