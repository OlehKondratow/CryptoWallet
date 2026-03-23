/**
 ******************************************************************************
 * @file    rng_dump.c
 * @brief   RNG data capture and UART output for Dieharder testing (USE_RNG_DUMP)
 ******************************************************************************
 * @details
 *          When USE_RNG_DUMP=1, outputs raw RNG data on UART3 in binary format.
 *          This file provides the task that continuously dumps RNG entropy
 *          to the serial port at 115200 baud for external Dieharder analysis.
 *
 *          **Architecture:**
 *          - RNG_Dump_Task: FreeRTOS task that generates and outputs RNG data
 *          - Other tasks may still log to the same UART — stream can mix binary and text
 *            (bad for dieharder purity; CI skips text markers when USE_RNG_DUMP=1)
 *
 *          **Usage:**
 *          - Build with: make USE_RNG_DUMP=1
 *          - Flash to device: make flash
 *          - Capture on PC: python3 scripts/capture_rng_uart.py --out rng.bin --bytes ...
 *          - CI: see documentation/06-integrity-rng-verification.md — default build uses USE_RNG_DUMP=1 for TRNG capture.
 *          - Test: dieharder -a -g 201 -f rng.bin
 ******************************************************************************
 */

#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>
#include "stm32h7xx_hal.h"
#include "stm32h7xx_hal_rng.h"
#include "crypto_wallet.h"
#include "hw_init.h"

#ifdef USE_RNG_DUMP

extern UART_HandleTypeDef huart3;
extern RNG_HandleTypeDef hrng;

/**
 * @brief   RNG Data Dump Task
 * @details Continuously generates and outputs RNG data to UART3 in binary format
 * @param   pvParameters: Task parameters (unused)
 */
void RNG_Dump_Task(void *pvParameters)
{
    (void)pvParameters;
    
    /* Buffer for RNG data: 256 bytes per iteration */
    uint8_t rng_buffer[256];
    
    /* Infinite loop: generate and transmit RNG data */
    while (1) {
        /* Fill buffer with STM32 hardware RNG data */
        for (size_t i = 0; i < sizeof(rng_buffer); i += 4) {
            /* Read raw RNG data from DR register (32-bit at a time) */
            uint32_t rng_val = RNG->DR;
            
            /* Store 4 bytes (or less if near end) */
            size_t n = sizeof(rng_buffer) - i;
            if (n > 4) n = 4;
            memcpy(rng_buffer + i, &rng_val, n);
        }
        
        /* Transmit buffer to UART3 (same mutex as logs / CWUP TX) */
        UART_Tx_Lock();
        (void)HAL_UART_Transmit(&huart3, rng_buffer, sizeof(rng_buffer), 1000);
        UART_Tx_Unlock();
        
        /* Small delay to prevent CPU saturation and allow other tasks */
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/**
 * @brief   Create RNG Dump Task
 * @details Called from main() when USE_RNG_DUMP is enabled
 * @return  pdPASS if successful, pdFAIL otherwise
 */
BaseType_t RNG_Dump_Task_Create(void)
{
    BaseType_t xReturn = xTaskCreate(
        RNG_Dump_Task,           /* Task function */
        "RNG_Dump",              /* Task name */
        configMINIMAL_STACK_SIZE * 2,  /* Stack size */
        NULL,                    /* Parameters */
        tskIDLE_PRIORITY + 1,    /* Priority */
        NULL                     /* Task handle */
    );
    
    return xReturn;
}

#endif /* USE_RNG_DUMP */
