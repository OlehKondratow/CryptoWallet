/**
  ******************************************************************************
  * @file    usb_device.c
  * @brief   USB device initialization for CryptoWallet WebUSB.
  ******************************************************************************
  */

#include "usb_device.h"
#include "usbd_core.h"
#include "usbd_desc_cw.h"
#include "usb_webusb.h"
#include "main.h"

#if defined(USE_WEBUSB) && (USE_WEBUSB == 1)

USBD_HandleTypeDef hUsbDeviceFS;
extern PCD_HandleTypeDef hpcd_USB_FS;

static void USBD_Clock_Config(void)
{
    RCC_PeriphCLKInitTypeDef pclk = {0};
    RCC_OscInitTypeDef osc = {0};

    pclk.PeriphClockSelection = RCC_PERIPHCLK_USB;
    pclk.UsbClockSelection = RCC_USBCLKSOURCE_HSI48;
    if (HAL_RCCEx_PeriphCLKConfig(&pclk) != HAL_OK) {
        Error_Handler();
    }

    osc.OscillatorType = RCC_OSCILLATORTYPE_HSI48;
    osc.HSI48State = RCC_HSI48_ON;
    osc.PLL.PLLState = RCC_PLL_NONE;
    if (HAL_RCC_OscConfig(&osc) != HAL_OK) {
        Error_Handler();
    }
}

void MX_USB_Device_Init(void)
{
    USBD_Clock_Config();

    if (USBD_Init(&hUsbDeviceFS, &WEBUSB_Desc, USBD_SPEED_FULL) != USBD_OK) {
        Error_Handler();
    }
    if (USBD_RegisterClass(&hUsbDeviceFS, &USBD_WEBUSB_ClassDriver) != USBD_OK) {
        Error_Handler();
    }
    if (USBD_Start(&hUsbDeviceFS) != USBD_OK) {
        Error_Handler();
    }
}

#endif /* USE_WEBUSB */
