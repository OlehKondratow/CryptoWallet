/**
 ******************************************************************************
 * @file    fault_report.c
 * @brief   Обработчики NMI / Hard / Mem / Bus / Usage: дамп регистров и SCB.
 ******************************************************************************
 */

#include "fault_report.h"
#include "hw_init.h"
#include "main.h"

#include <stdio.h>

extern UART_HandleTypeDef huart3;

static void fault_uart_line_isr(const char *line)
{
    if (line == NULL) {
        return;
    }
    if (huart3.Instance == NULL) {
        return;
    }
    size_t n = 0;
    while (line[n] != '\0' && n < 256U) {
        n++;
    }
    if (n == 0U) {
        return;
    }
    (void)HAL_UART_Transmit(&huart3, (const uint8_t *)line, (uint16_t)n, 1000);
    static const char crlf[] = "\r\n";
    (void)HAL_UART_Transmit(&huart3, (const uint8_t *)crlf, 2U, 100);
}

void Fault_ConfigAssertFailed(const char *file, int line)
{
    char buf[160];
    (void)snprintf(buf, sizeof(buf), "[ERR] configASSERT %s:%d", file != NULL ? file : "?", line);
    fault_uart_line_isr(buf);
    for (;;) {
        __NOP();
    }
}

void Fault_Report_Init(void)
{
    /* Включить отдельные fault handler’ы (иначе эскалация в HardFault). */
    SCB->SHCSR |= (uint32_t)(SCB_SHCSR_MEMFAULTENA_Msk | SCB_SHCSR_BUSFAULTENA_Msk |
                             SCB_SHCSR_USGFAULTENA_Msk);
    /* Ловить деление на ноль (если компилятор генерирует SDIV/UDIV). */
    SCB->CCR |= (uint32_t)SCB_CCR_DIV_0_TRP_Msk;
}

static void fault_print_hex_isr(const char *label, uint32_t v)
{
    char buf[48];
    (void)snprintf(buf, sizeof(buf), "[ERR] [FAULT] %s=%08lX", label, (unsigned long)v);
    fault_uart_line_isr(buf);
}

void Fault_Handler_C(uint32_t *sp, uint32_t kind)
{
    static const char *const names[] = {"HARD", "MEM", "BUS", "USAGE"};
    const char *tag = "???";
    if (kind < 4U) {
        tag = names[kind];
    }

    fault_uart_line_isr("[ERR] [FAULT] ==========");
    {
        char head[64];
        (void)snprintf(head, sizeof(head), "[ERR] [FAULT] kind=%s", tag);
        fault_uart_line_isr(head);
    }

    fault_print_hex_isr("CFSR", SCB->CFSR);
    fault_print_hex_isr("HFSR", SCB->HFSR);
    fault_print_hex_isr("MMFAR", SCB->MMFAR);
    fault_print_hex_isr("BFAR", SCB->BFAR);
    fault_print_hex_isr("AFSR", SCB->AFSR);

    if (sp != NULL) {
        fault_print_hex_isr("R0", sp[0]);
        fault_print_hex_isr("R1", sp[1]);
        fault_print_hex_isr("R2", sp[2]);
        fault_print_hex_isr("R3", sp[3]);
        fault_print_hex_isr("R12", sp[4]);
        fault_print_hex_isr("LR", sp[5]);
        fault_print_hex_isr("PC", sp[6]);
        fault_print_hex_isr("xPSR", sp[7]);
    } else {
        fault_uart_line_isr("[ERR] [FAULT] sp=NULL");
    }

    fault_uart_line_isr("[ERR] [FAULT] halt");
    for (;;) {
        __NOP();
    }
}

void NMI_Handler(void)
{
    fault_uart_line_isr("[ERR] [FAULT] NMI");
    for (;;) {
        __NOP();
    }
}

void DebugMon_Handler(void)
{
    fault_uart_line_isr("[ERR] [FAULT] DebugMon");
    for (;;) {
        __NOP();
    }
}

void HardFault_Handler(void) __attribute__((naked, notreached));
void HardFault_Handler(void)
{
    __asm volatile("mov r1, #0 \n"
                   "tst lr, #4 \n"
                   "ite eq \n"
                   "mrseq r0, msp \n"
                   "mrsne r0, psp \n"
                   "b Fault_Handler_C \n");
}

void MemManage_Handler(void) __attribute__((naked));
void MemManage_Handler(void)
{
    __asm volatile("mov r1, #1 \n"
                   "tst lr, #4 \n"
                   "ite eq \n"
                   "mrseq r0, msp \n"
                   "mrsne r0, psp \n"
                   "b Fault_Handler_C \n");
}

void BusFault_Handler(void) __attribute__((naked));
void BusFault_Handler(void)
{
    __asm volatile("mov r1, #2 \n"
                   "tst lr, #4 \n"
                   "ite eq \n"
                   "mrseq r0, msp \n"
                   "mrsne r0, psp \n"
                   "b Fault_Handler_C \n");
}

void UsageFault_Handler(void) __attribute__((naked));
void UsageFault_Handler(void)
{
    __asm volatile("mov r1, #3 \n"
                   "tst lr, #4 \n"
                   "ite eq \n"
                   "mrseq r0, msp \n"
                   "mrsne r0, psp \n"
                   "b Fault_Handler_C \n");
}
