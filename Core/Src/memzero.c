/**
  ******************************************************************************
  * @file    memzero.c
  * @brief   Secure @c memzero() — volatile byte writes (no optimize-out).
  ******************************************************************************
  * @details
  *          **Role:** Clear seeds, digests, session buffers after use (signing stack,
  *          @c task_security.c , @c wallet_seed.c , crypto paths). Same pattern as
  *          common crypto libraries on bare metal.
  ******************************************************************************
  */

#include "memzero.h"

/**
 * @brief   Secure buffer zeroing (see @c memzero.h ).
 * @details Volatile pointer loop so the store chain is not optimized away.
 */
void memzero(void *pnt, size_t len)
{
    volatile unsigned char *p = (volatile unsigned char *)pnt;
    while (len--) {
        *p++ = 0U;
    }
}
