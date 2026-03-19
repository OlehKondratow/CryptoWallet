/**
  ******************************************************************************
  * @file    memzero.h
  * @brief   Secure memory zeroing (prevents compiler optimization).
  ******************************************************************************
  * @details Use for clearing sensitive data (keys, digest, signatures).
  *          Pattern from Trezor/crypto and libsodium.
  ******************************************************************************
  */

#ifndef __MEMZERO_H__
#define __MEMZERO_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

/**
 * @brief   Overwrite a buffer with zeros using volatile writes.
 * @details Prevents the compiler from removing the loop (unlike @c memset in some LTO builds).
 *          Use after handling seeds, private keys, digests, or temporary signature buffers.
 * @param   pnt  Start address; may be any alignment (byte-wise write).
 * @param   len  Number of bytes to clear; zero is a no-op.
 */
void memzero(void *pnt, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* __MEMZERO_H__ */
