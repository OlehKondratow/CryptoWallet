/**
  ******************************************************************************
  * @file    stm32h7xx_it_systick.c
  * @brief   SysTick handler for minimal-lwip (FreeRTOS tick).
  ******************************************************************************
  * @details LWIP_APP stm32h7xx_it.c lacks SysTick_Handler; without it,
  *          SysTick goes to Default_Handler → infinite loop / "WWDG" backtrace.
  ******************************************************************************
  */

#include "stm32h7xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"

extern void xPortSysTickHandler(void);

/**
  * @brief  SysTick ISR - HAL tick + FreeRTOS tick.
  * @retval None
  */
void SysTick_Handler(void)
{
    HAL_IncTick();
    xPortSysTickHandler();
}
