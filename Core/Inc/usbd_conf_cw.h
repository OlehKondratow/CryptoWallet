/**
  ******************************************************************************
  * @file    usbd_conf_cw.h
  * @brief   USB device BSP configuration for CryptoWallet WebUSB.
  ******************************************************************************
  */

#ifndef __USBD_CONF_CW_H__
#define __USBD_CONF_CW_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define USBD_MAX_NUM_INTERFACES     1U
#define USBD_MAX_NUM_CONFIGURATION 1U
#define USBD_MAX_STR_DESC_SIZ      0x100U
#define USBD_SELF_POWERED          1U
#define USBD_DEBUG_LEVEL           0U
#define USBD_CLASS_BOS_ENABLED     1U

#define USBD_malloc    (void *)USBD_static_malloc
#define USBD_free      USBD_static_free
#define USBD_memset    memset
#define USBD_memcpy    memcpy
#define USBD_Delay     HAL_Delay

#define USBD_UsrLog(...)
#define USBD_ErrLog(...)
#define USBD_DbgLog(...)

void *USBD_static_malloc(uint32_t size);
void USBD_static_free(void *p);

#ifdef __cplusplus
}
#endif

#endif /* __USBD_CONF_CW_H__ */
