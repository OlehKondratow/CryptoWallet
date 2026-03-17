/**
  ******************************************************************************
  * @file    hw_init.h
  * @brief   Hardware initialization wrapper (CMSIS-compliant).
  ******************************************************************************
  * @details No hardware registers in main.c; all init via this wrapper.
  ******************************************************************************
  */

#ifndef __HW_INIT_H
#define __HW_INIT_H

#include "stm32h7xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief I2C1 handle for SSD1306 (defined in hw_init.c) */
extern I2C_HandleTypeDef hi2c1;

/** @brief USART3 handle for debug log (PD8/PD9, 115200) */
extern UART_HandleTypeDef huart3;

/** @brief Send string to UART (no newline). */
void UART_Log(const char *msg);

#ifdef USE_LWIP
/** @brief MPU + Cache init for LwIP — call BEFORE HAL_Init() (same order as lwip_zero). */
void HW_Init_Early_LwIP(void);
#endif

/**
 * @brief Initialize all hardware (clock, MPU, cache, GPIO, I2C1).
 * @note  Called from main() before FreeRTOS scheduler start.
 * @return None.
 */
void HW_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* __HW_INIT_H */
