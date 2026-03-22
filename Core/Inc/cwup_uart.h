/**
 * @file cwup_uart.h
 * @brief CWUP-0.1: UART3 command RX (AT+) and text responses. Full logic is compiled out when USE_RNG_DUMP=1 (same UART = binary TRNG).
 */
#ifndef CWUP_UART_H
#define CWUP_UART_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create queue/task and start UART RX interrupt (call after IPC, before scheduler).
 */
void Cwup_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* CWUP_UART_H */
