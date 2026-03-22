/**
  ******************************************************************************
  * @file    hw_init.c
  * @brief   Board bring-up: clock, MPU/cache, GPIO, I2C1 (OLED), UART, optional USB.
  ******************************************************************************
  */

#include "main.h"
#include "hw_init.h"
#include "app_log.h"
#if defined(USE_WEBUSB) && (USE_WEBUSB == 1)
#include "usb_device.h"
#endif
#if (defined(USE_CRYPTO_SIGN) && (USE_CRYPTO_SIGN == 1)) || (defined(USE_RNG_DUMP) && (USE_RNG_DUMP == 1))
#include "stm32h7xx_hal_rng.h"
#endif
#include <stddef.h>
#if defined(USE_LWIP) && !SKIP_OLED
#include "ssd1306.h"
#include "ssd1306_fonts.h"
#endif

I2C_HandleTypeDef hi2c1;
#if (defined(USE_CRYPTO_SIGN) && (USE_CRYPTO_SIGN == 1)) || (defined(USE_RNG_DUMP) && (USE_RNG_DUMP == 1))
RNG_HandleTypeDef hrng;
#endif

static void SystemClock_Config(void);
static void CPU_CACHE_Enable(void);
static void MPU_Config(void);

#ifdef USE_LWIP
/**
 * @brief MPU + Cache init for LwIP before `HAL_Init()`.
 */
void HW_Init_Early_LwIP(void)
{
    MPU_Region_InitTypeDef MPU_InitStruct;

    HAL_MPU_Disable();

    /* Region 0: 4GB no-access default */
    MPU_InitStruct.Enable = MPU_REGION_ENABLE;
    MPU_InitStruct.BaseAddress = 0x00;
    MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
    MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
    MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
    MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
    MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
    MPU_InitStruct.Number = MPU_REGION_NUMBER0;
    MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
    MPU_InitStruct.SubRegionDisable = 0x87;
    MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
    HAL_MPU_ConfigRegion(&MPU_InitStruct);

    /* Region 1: ETH descriptors at 0x30000000 (1KB, device) */
    MPU_InitStruct.Enable = MPU_REGION_ENABLE;
    MPU_InitStruct.BaseAddress = 0x30000000;
    MPU_InitStruct.Size = MPU_REGION_SIZE_1KB;
    MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
    MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;
    MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
    MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
    MPU_InitStruct.Number = MPU_REGION_NUMBER1;
    MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
    MPU_InitStruct.SubRegionDisable = 0x00;
    MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;
    HAL_MPU_ConfigRegion(&MPU_InitStruct);

    /* Region 2: LwIP heap at 0x30004000 (16KB) */
    MPU_InitStruct.Enable = MPU_REGION_ENABLE;
    MPU_InitStruct.BaseAddress = 0x30004000;
    MPU_InitStruct.Size = MPU_REGION_SIZE_16KB;
    MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
    MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
    MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
    MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
    MPU_InitStruct.Number = MPU_REGION_NUMBER2;
    MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL1;
    MPU_InitStruct.SubRegionDisable = 0x00;
    MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;
    HAL_MPU_ConfigRegion(&MPU_InitStruct);

    HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

    SCB_EnableICache();
    SCB_EnableDCache();
}
#endif
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART3_Init(void);
#if defined(USE_WEBUSB) && (USE_WEBUSB == 1)
extern void MX_USB_Device_Init(void);
#endif
#if (defined(USE_CRYPTO_SIGN) && (USE_CRYPTO_SIGN == 1)) || (defined(USE_RNG_DUMP) && (USE_RNG_DUMP == 1))
static void MX_RNG_Init(void);
#endif

/**
 * @brief Initialize board peripherals after `HAL_Init()`.
 */
void HW_Init(void)
{
#ifndef USE_LWIP
    MPU_Config();
#endif
    /* Delay for ST-Link MCO to stabilize (Nucleo-H743ZI2) */
    for (volatile uint32_t i = 0; i < 2000000; i++) { (void)i; }
    SystemClock_Config();
    SystemCoreClockUpdate();
#ifndef USE_LWIP
    CPU_CACHE_Enable();  /* After clock switch — recommended for H7 */
#endif
    MX_GPIO_Init();
    MX_I2C1_Init();
#if defined(USE_LWIP) && !SKIP_OLED
    /* OLED init right after I2C — same order as lwip-uaid-SSD1306 BSP_Config */
    ssd1306_Init();
#endif
    MX_USART3_Init();
#if defined(USE_WEBUSB) && (USE_WEBUSB == 1)
    APP_LOG_INFO("[USB] init");
    MX_USB_Device_Init();
    APP_LOG_INFO("[USB] ready");
#endif
#if (defined(USE_CRYPTO_SIGN) && (USE_CRYPTO_SIGN == 1)) || (defined(USE_RNG_DUMP) && (USE_RNG_DUMP == 1))
    MX_RNG_Init();
#endif
}

#if (defined(USE_CRYPTO_SIGN) && (USE_CRYPTO_SIGN == 1)) || (defined(USE_RNG_DUMP) && (USE_RNG_DUMP == 1))
void HAL_RNG_MspInit(RNG_HandleTypeDef *rng_handle)
{
    (void)rng_handle;
    __HAL_RCC_RNG_CLK_ENABLE();
}

static void MX_RNG_Init(void)
{
    hrng.Instance = RNG;
    hrng.Init.ClockErrorDetection = RNG_CED_ENABLE;
    if (HAL_RNG_Init(&hrng) != HAL_OK) {
        /* RNG init failed — crypto_rng_init will use fallback */
    }
}
#endif

static void SystemClock_Config(void)
{
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};

    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
    while (!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

    /* Enable D2 SRAM3 clock (RAM_D2 at 0x30000000) — required for LwIP ETH DMA */
    __HAL_RCC_D2SRAM3_CLK_ENABLE();

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState       = RCC_HSE_BYPASS;
    RCC_OscInitStruct.HSIState       = RCC_HSI_OFF;
    RCC_OscInitStruct.CSIState       = RCC_CSI_OFF;
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM       = 4;
    RCC_OscInitStruct.PLL.PLLN       = 400;
    RCC_OscInitStruct.PLL.PLLFRACN   = 0;
    RCC_OscInitStruct.PLL.PLLP       = 2;
    RCC_OscInitStruct.PLL.PLLR       = 2;
    RCC_OscInitStruct.PLL.PLLQ       = 4;
    RCC_OscInitStruct.PLL.PLLVCOSEL  = RCC_PLL1VCOWIDE;
    RCC_OscInitStruct.PLL.PLLRGE     = RCC_PLL1VCIRANGE_1;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK |
                                   RCC_CLOCKTYPE_D1PCLK1 | RCC_CLOCKTYPE_PCLK1 |
                                   RCC_CLOCKTYPE_PCLK2 | RCC_CLOCKTYPE_D3PCLK1);
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.SYSCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
    RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK) {
        Error_Handler();
    }
}

static void CPU_CACHE_Enable(void)
{
    SCB_EnableICache();
    SCB_EnableDCache();
}

static void MPU_Config(void)
{
    HAL_MPU_Disable();
}

static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef g = {0};

    LED1_GPIO_CLK_ENABLE();
    LED2_GPIO_CLK_ENABLE();
    LED3_GPIO_CLK_ENABLE();
    USER_KEY_GPIO_CLK_ENABLE();

    g.Mode  = GPIO_MODE_OUTPUT_PP;
    g.Pull  = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_LOW;

    g.Pin = LED1_PIN;
    HAL_GPIO_Init(LED1_GPIO_PORT, &g);
    g.Pin = LED2_PIN;
    HAL_GPIO_Init(LED2_GPIO_PORT, &g);
    g.Pin = LED3_PIN;
    HAL_GPIO_Init(LED3_GPIO_PORT, &g);

    HAL_GPIO_WritePin(LED1_GPIO_PORT, LED1_PIN, LED1_OFF_LEVEL);
    HAL_GPIO_WritePin(LED2_GPIO_PORT, LED2_PIN, LED2_OFF_LEVEL);
    HAL_GPIO_WritePin(LED3_GPIO_PORT, LED3_PIN, LED3_OFF_LEVEL);

    g.Pin   = USER_KEY_PIN;
    g.Mode  = GPIO_MODE_INPUT;
    g.Pull  = GPIO_NOPULL;
    HAL_GPIO_Init(USER_KEY_GPIO_PORT, &g);
}

static void MX_I2C1_Init(void)
{
    /* Same as lwip-uaid-SSD1306: no reset, no explicit GPIO — MSP handles it */
    hi2c1.Instance             = I2Cx;
    hi2c1.Init.Timing          = 0x10707DBC;  /* Same as stm32_secure_boot i2c-SSD1306 */
    hi2c1.Init.OwnAddress1     = 0;
    hi2c1.Init.AddressingMode  = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2     = 0;
    hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode   = I2C_NOSTRETCH_DISABLE;

    if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
        Error_Handler();
    }
    HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE);
    HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0);
}

UART_HandleTypeDef huart3;

static void MX_USART3_Init(void)
{
    huart3.Instance = USARTx;
    huart3.Init.BaudRate = 115200;
    huart3.Init.WordLength = UART_WORDLENGTH_8B;
    huart3.Init.StopBits = UART_STOPBITS_1;
    huart3.Init.Parity = UART_PARITY_NONE;
    huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart3.Init.Mode = UART_MODE_TX_RX;
    huart3.Init.OverSampling = UART_OVERSAMPLING_16;
    huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

    if (HAL_UART_Init(&huart3) != HAL_OK) {
        Error_Handler();
    }
}

void UART_Log(const char *msg)
{
    if (msg == NULL || huart3.Instance == NULL) return;
    size_t n = 0;
    while (msg[n] != '\0' && n < 256) n++;
    if (n == 0) return;
    HAL_UART_Transmit(&huart3, (const uint8_t *)msg, (uint16_t)n, 500);
}
