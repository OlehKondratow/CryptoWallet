/**
  ******************************************************************************
  * @file    memzero.c
  * @brief   Secure memory zeroing for embedded (volatile write).
  ******************************************************************************
  */

#include "memzero.h"

void memzero(void *pnt, size_t len)
{
    volatile unsigned char *p = (volatile unsigned char *)pnt;
    while (len--) {
        *p++ = 0U;
    }
}
