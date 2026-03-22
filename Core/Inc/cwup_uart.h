/**
 * @file cwup_uart.h
 * @brief CWUP-0.1: UART3 command RX (AT+) and text responses. Full logic is compiled out when USE_RNG_DUMP=1 (same UART = binary TRNG).
 */
#ifndef CWUP_UART_H
#define CWUP_UART_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief CWUP line lifecycle (UART_PROTOCOL_MVP §3).
 * @details Phase B is the single @c CW+READY line (not stored as a lasting state). Phase D is reserved until @c AT+RNG=START / framed TRNG is implemented.
 */
typedef enum {
    CWUP_PHASE_A_PRE_READY = 0, /**< Before first @c CW+READY: TEXT preamble/logs only; no §7 TRNG frames. */
    CWUP_PHASE_C_TEXT_CMD = 1,  /**< After @c CW+READY: TEXT @c AT+... / @c CW+... */
    CWUP_PHASE_D_BINARY_RNG = 2 /**< Framed TRNG §7 on TX (not implemented in current firmware). */
} cwup_line_phase_t;

/**
 * @brief Create queue/task and start UART RX interrupt (call after IPC, before scheduler).
 */
void Cwup_Init(void);

/**
 * @brief Current CWUP UART line phase (for diagnostics/tests; ISR-safe read of a single byte-sized enum).
 */
cwup_line_phase_t Cwup_GetLinePhase(void);

#ifdef __cplusplus
}
#endif

#endif /* CWUP_UART_H */
