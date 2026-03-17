/**
  ******************************************************************************
  * @file    stm32h7xx_it.c
  * @brief   Interrupt handlers - FreeRTOS SysTick, ETH (when USE_LWIP).
  ******************************************************************************
  */

#include "main.h"

extern void xPortSysTickHandler(void);

#ifdef USE_LWIP
extern ETH_HandleTypeDef EthHandle;
#endif

/**
 * @brief SysTick handler - HAL tick + FreeRTOS tick.
 * @note  HAL_IncTick required for HAL_Delay (used by ssd1306_Init).
 */
void SysTick_Handler(void)
{
    HAL_IncTick();
    xPortSysTickHandler();
}

#ifdef USE_LWIP
void ETH_IRQHandler(void)
{
    HAL_ETH_IRQHandler(&EthHandle);
}
#endif
