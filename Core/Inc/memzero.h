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
 * @brief Securely zero memory. Compiler cannot optimize away.
 * @param pnt  Pointer to buffer.
 * @param len  Length in bytes.
 */
void memzero(void *pnt, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* __MEMZERO_H__ */
