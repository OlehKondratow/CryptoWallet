/**
  ******************************************************************************
  * @file    main.h
  * @brief   Board pins, LEDs, UART logging macro, network IP defaults.
  ******************************************************************************
  * @details
  *          STM32H743ZI2 (Nucleo-144). **Documentation index:** repository @c README.md .
  *          **Pinout table:** @c Core/Src/hw_init.c and @c docs_src/pinout.md .
  *
  *          Adjust @c IP_ADDR0..3 , netmask, gateway for your LAN; DHCP vs static in
  *          @c Core/Inc/lwipopts.h .
  ******************************************************************************
  */

#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"

/*-----------------------------------------------------------------------------
 * LED definitions (NUCLEO-H743ZI2)
 *-----------------------------------------------------------------------------*/
/** @brief LED1: Green - System OK (PB0, active-low) */
#define LED1_PIN                    GPIO_PIN_0
#define LED1_GPIO_PORT              GPIOB
#define LED1_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOB_CLK_ENABLE()
#define LED1_ON_LEVEL               GPIO_PIN_RESET
#define LED1_OFF_LEVEL              GPIO_PIN_SET

/** @brief LED2: Yellow - Network status (PE1, active-low) */
#define LED2_PIN                    GPIO_PIN_1
#define LED2_GPIO_PORT              GPIOE
#define LED2_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOE_CLK_ENABLE()
#define LED2_ON_LEVEL                GPIO_PIN_RESET
#define LED2_OFF_LEVEL               GPIO_PIN_SET

/** @brief LED3: Red - Security Alert (PE2, active-low) */
#define LED3_PIN                    GPIO_PIN_2
#define LED3_GPIO_PORT              GPIOE
#define LED3_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOE_CLK_ENABLE()
#define LED3_ON_LEVEL                GPIO_PIN_RESET
#define LED3_OFF_LEVEL               GPIO_PIN_SET

/*-----------------------------------------------------------------------------
 * User Key (Confirm button) - PC13
 *-----------------------------------------------------------------------------*/
#define USER_KEY_PIN                GPIO_PIN_13
#define USER_KEY_GPIO_PORT          GPIOC
#define USER_KEY_GPIO_CLK_ENABLE()  __HAL_RCC_GPIOC_CLK_ENABLE()
/** @brief User button is active-low (pressed = GPIO low) */
#define USER_KEY_PRESSED            GPIO_PIN_RESET

/*-----------------------------------------------------------------------------
 * I2C1 for SSD1306 OLED (PB8 SCL, PB9 SDA)
 *-----------------------------------------------------------------------------*/
#define I2Cx                            I2C1
#define I2Cx_CLK_ENABLE()               __HAL_RCC_I2C1_CLK_ENABLE()
#define I2Cx_GPIO_CLK_ENABLE()          __HAL_RCC_GPIOB_CLK_ENABLE()
#define I2Cx_GPIO_PORT                  GPIOB
#define I2Cx_AF                         GPIO_AF4_I2C1
#define I2Cx_SCL_PIN                    GPIO_PIN_8
#define I2Cx_SDA_PIN                    GPIO_PIN_9
#define SSD1306_I2C_ADDR                (0x3C << 1)

/*-----------------------------------------------------------------------------
 * USART3: Virtual COM (PD8 TX, PD9 RX), 115200 8N1
 *-----------------------------------------------------------------------------*/
#define USARTx                           USART3
#define USARTx_CLK_ENABLE()              __HAL_RCC_USART3_CLK_ENABLE()
#define USARTx_TX_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOD_CLK_ENABLE()
#define USARTx_RX_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOD_CLK_ENABLE()
#define USARTx_TX_PIN                    GPIO_PIN_8
#define USARTx_TX_GPIO_PORT              GPIOD
#define USARTx_TX_AF                     GPIO_AF7_USART3
#define USARTx_RX_PIN                    GPIO_PIN_9
#define USARTx_RX_GPIO_PORT              GPIOD
#define USARTx_RX_AF                     GPIO_AF7_USART3

/*-----------------------------------------------------------------------------
 * Ethernet (static IP fallback)
 *-----------------------------------------------------------------------------*/
#define IP_ADDR0    ((uint8_t)192U)
#define IP_ADDR1    ((uint8_t)168U)
#define IP_ADDR2    ((uint8_t)0U)
#define IP_ADDR3    ((uint8_t)10U)
#define NETMASK_ADDR0   ((uint8_t)255U)
#define NETMASK_ADDR1   ((uint8_t)255U)
#define NETMASK_ADDR2   ((uint8_t)255U)
#define NETMASK_ADDR3   ((uint8_t)0U)
#define GW_ADDR0        ((uint8_t)192U)
#define GW_ADDR1        ((uint8_t)168U)
#define GW_ADDR2        ((uint8_t)0U)
#define GW_ADDR3        ((uint8_t)1U)

/*-----------------------------------------------------------------------------
 * Function prototypes
 *-----------------------------------------------------------------------------*/

/**
 * @brief Fatal error handler — log and infinite loop.
 * @return Never returns.
 */
void Error_Handler(void);

/**
 * @brief Как Error_Handler, но с указанием места в коде (файл:строка).
 * @param file Имя файла или NULL для обобщённого сообщения.
 * @param line Номер строки; при @a file == NULL игнорируется.
 */
void Error_Handler_At(const char *file, int line);

/** Остановка с логом @c Error_Handler_At(__FILE__, __LINE__). */
#define ERROR_HALT() Error_Handler_At(__FILE__, __LINE__)

/**
 * @brief Hardware initialization (clock, MPU, GPIO, I2C1, UART).
 * @note  Called from main() before vTaskStartScheduler().
 * @return None.
 */
void HW_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
