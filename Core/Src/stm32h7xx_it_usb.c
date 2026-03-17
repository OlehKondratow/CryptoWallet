/**
  ******************************************************************************
  * @file    stm32h7xx_it_usb.c
  * @brief   USB OTG HS interrupt handler (WebUSB).
  ******************************************************************************
  */

#include "stm32h7xx_hal.h"

#if defined(USE_WEBUSB) && (USE_WEBUSB == 1)

extern PCD_HandleTypeDef hpcd_USB_FS;

void OTG_HS_IRQHandler(void)
{
    HAL_PCD_IRQHandler(&hpcd_USB_FS);
}

#endif /* USE_WEBUSB */
