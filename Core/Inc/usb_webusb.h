/**
  ******************************************************************************
  * @file    usb_webusb.h
  * @brief   WebUSB vendor-specific class for CryptoWallet.
  ******************************************************************************
  * @details Vendor-specific interface (0xFF) with bulk endpoints.
  *          Compatible with Chrome WebUSB API.
  ******************************************************************************
  */

#ifndef __USB_WEBUSB_H__
#define __USB_WEBUSB_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "usbd_ioreq.h"

#define WEBUSB_EPIN_ADDR      0x81U
#define WEBUSB_EPOUT_ADDR     0x02U
#define WEBUSB_EP_SIZE        64U

#define WEBUSB_CONFIG_DESC_SIZ  (9 + 9 + 7 + 7)  /* Config + IF + EP_IN + EP_OUT */

extern USBD_ClassTypeDef USBD_WEBUSB_ClassDriver;

#if defined(USE_WEBUSB) && (USE_WEBUSB == 1)
/** Call from task_sign when g_last_sig_ready is set. */
void WebUSB_NotifySignatureReady(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __USB_WEBUSB_H__ */
