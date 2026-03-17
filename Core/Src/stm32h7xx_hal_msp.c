/**
  ******************************************************************************
  * @file    stm32h7xx_hal_msp.c
  * @brief   MSP init for CryptoWallet - I2C1 (SSD1306).
  ******************************************************************************
  */

#include "main.h"

void HAL_MspInit(void)
{
}

void HAL_I2C_MspInit(I2C_HandleTypeDef *hi2c)
{
    GPIO_InitTypeDef g = {0};

    if (hi2c->Instance == I2Cx) {
        I2Cx_GPIO_CLK_ENABLE();
        I2Cx_CLK_ENABLE();
        g.Pin       = I2Cx_SCL_PIN | I2Cx_SDA_PIN;
        g.Mode      = GPIO_MODE_AF_OD;
        g.Pull      = GPIO_PULLUP;
        g.Speed     = GPIO_SPEED_FREQ_LOW;
        g.Alternate = I2Cx_AF;
        HAL_GPIO_Init(I2Cx_GPIO_PORT, &g);
    }
}

void HAL_I2C_MspDeInit(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance == I2Cx) {
        __HAL_RCC_I2C1_FORCE_RESET();
        __HAL_RCC_I2C1_RELEASE_RESET();
        HAL_GPIO_DeInit(I2Cx_GPIO_PORT, I2Cx_SCL_PIN | I2Cx_SDA_PIN);
    }
}

void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef g = {0};
    RCC_PeriphCLKInitTypeDef pclk = {0};

    if (huart->Instance == USARTx) {
        pclk.PeriphClockSelection = RCC_PERIPHCLK_USART234578;
        pclk.Usart234578ClockSelection = RCC_USART234578CLKSOURCE_PCLK1;
        if (HAL_RCCEx_PeriphCLKConfig(&pclk) != HAL_OK) Error_Handler();

        USARTx_TX_GPIO_CLK_ENABLE();
        USARTx_RX_GPIO_CLK_ENABLE();
        USARTx_CLK_ENABLE();

        g.Pin       = USARTx_TX_PIN;
        g.Mode      = GPIO_MODE_AF_PP;
        g.Pull      = GPIO_PULLUP;
        g.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
        g.Alternate = USARTx_TX_AF;
        HAL_GPIO_Init(USARTx_TX_GPIO_PORT, &g);

        g.Pin       = USARTx_RX_PIN;
        g.Alternate = USARTx_RX_AF;
        HAL_GPIO_Init(USARTx_RX_GPIO_PORT, &g);
    }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USARTx) {
        __HAL_RCC_USART3_FORCE_RESET();
        __HAL_RCC_USART3_RELEASE_RESET();
        HAL_GPIO_DeInit(USARTx_TX_GPIO_PORT, USARTx_TX_PIN);
        HAL_GPIO_DeInit(USARTx_RX_GPIO_PORT, USARTx_RX_PIN);
    }
}
