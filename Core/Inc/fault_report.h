/**
 ******************************************************************************
 * @file    fault_report.h
 * @brief   Cortex-M fault logging (UART без mutex) и инициализация SCB.
 ******************************************************************************
 * @details Логирование из ISR идёт через HAL_UART_Transmit на huart3 — не
 *          вызывать UART_Log / Task_Display_Log (мьютекс FreeRTOS).
 ******************************************************************************
 */

#ifndef FAULT_REPORT_H
#define FAULT_REPORT_H

#include <stdint.h>

void Fault_Report_Init(void);

/** Вызывается из configASSERT (FreeRTOS): лог + бесконечный цикл. */
void Fault_ConfigAssertFailed(const char *file, int line);

#endif /* FAULT_REPORT_H */
