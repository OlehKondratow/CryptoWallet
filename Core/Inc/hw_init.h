/**
  ******************************************************************************
  * @file    hw_init.h
  * @brief   Hardware initialization wrapper (CMSIS-compliant).
  ******************************************************************************
  */

#ifndef __HW_INIT_H
#define __HW_INIT_H

#include "stm32h7xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief HAL I2C1 instance bound to SSD1306 (PB8/PB9); defined in @c hw_init.c . */
extern I2C_HandleTypeDef hi2c1;

/** @brief USART3 for debug logging (115200, pins in @c main.h / MSP). */
extern UART_HandleTypeDef huart3;

/**
 * @brief   Blocking UART print of a C string (no CRLF appended).
 */
void UART_Log(const char *msg);

#ifdef USE_LWIP
/**
 * @brief   Configure MPU regions and caches before @c HAL_Init() for Ethernet/DMA safety.
 */
void HW_Init_Early_LwIP(void);
#endif

/**
 * @brief   Full board setup after @c HAL_Init() : clocks, GPIO, I2C1, UART3, optional USB.
 */
void HW_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* __HW_INIT_H */
