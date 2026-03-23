/**
 ******************************************************************************
 * @file    stm32h7xx_it_lwip.c
 * @brief   Ethernet IRQ for minimal-lwip (fault handlers live in fault_report.c).
 ******************************************************************************
 * @details Previously $(LWIP_APP)/Src/stm32h7xx_it.c was used with empty fault
 *          loops; the build now takes this file from the CryptoWallet repository.
 ******************************************************************************
 */

#include "stm32h7xx_hal.h"
#include "stm32h7xx_hal_uart.h"
#include "main.h"

extern ETH_HandleTypeDef EthHandle;
extern UART_HandleTypeDef huart3;

void ETH_IRQHandler(void)
{
    HAL_ETH_IRQHandler(&EthHandle);
}

void USART3_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart3);
}
