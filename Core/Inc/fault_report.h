/**
 ******************************************************************************
 * @file    fault_report.h
 * @brief   Cortex-M fault logging (UART without mutex) and SCB initialization.
 ******************************************************************************
 * @details Logging from ISR uses HAL_UART_Transmit on huart3 — do not call
 *          UART_Log / Task_Display_Log (FreeRTOS mutex).
 ******************************************************************************
 */

#ifndef FAULT_REPORT_H
#define FAULT_REPORT_H

#include <stdint.h>

void Fault_Report_Init(void);

/** Called from configASSERT (FreeRTOS): log then infinite loop. */
void Fault_ConfigAssertFailed(const char *file, int line);

#endif /* FAULT_REPORT_H */
