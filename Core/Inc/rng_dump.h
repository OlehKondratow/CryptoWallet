/**
 ******************************************************************************
 * @file    rng_dump.h
 * @brief   RNG data capture and UART output for Dieharder testing (USE_RNG_DUMP)
 ******************************************************************************
 */

#ifndef RNG_DUMP_H
#define RNG_DUMP_H

#include "FreeRTOS.h"

#ifdef USE_RNG_DUMP

/**
 * @brief   Create RNG Dump Task
 * @details Called from main() when USE_RNG_DUMP is enabled.
 *          Continuously outputs raw RNG data to UART3 at 115200 baud.
 * @return  pdPASS if successful, pdFAIL otherwise
 */
BaseType_t RNG_Dump_Task_Create(void);

#else

/* Stub when USE_RNG_DUMP is not defined */
#define RNG_Dump_Task_Create() (pdPASS)

#endif /* USE_RNG_DUMP */

#endif /* RNG_DUMP_H */
