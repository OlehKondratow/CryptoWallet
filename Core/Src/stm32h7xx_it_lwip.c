/**
 ******************************************************************************
 * @file    stm32h7xx_it_lwip.c
 * @brief   Ethernet IRQ для minimal-lwip (fault’ы — в fault_report.c).
 ******************************************************************************
 * @details Раньше использовался $(LWIP_APP)/Src/stm32h7xx_it.c с пустыми fault
 *          loop; теперь сборка берёт этот файл из репозитория CryptoWallet.
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
