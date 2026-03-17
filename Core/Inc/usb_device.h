/**
  ******************************************************************************
  * @file    usb_device.h
  * @brief   USB device init for CryptoWallet WebUSB.
  ******************************************************************************
  */

#ifndef __USB_DEVICE_H__
#define __USB_DEVICE_H__

#ifdef __cplusplus
extern "C" {
#endif

#if defined(USE_WEBUSB) && (USE_WEBUSB == 1)
void MX_USB_Device_Init(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __USB_DEVICE_H__ */
