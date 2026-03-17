# API Reference

## Core

### hw_init.h

- `void HW_Init(void)` — Clock, MPU, GPIO, I2C1, UART
- `void HW_Init_Early_LwIP(void)` — MPU + Cache before HAL (LwIP only)
- `void UART_Log(const char *msg)` — Log to UART

### task_net.h

- `void Task_Net_Create(void)` — Create network task (LwIP HTTP server)

### task_display.h

- `void Task_Display_Create(void)` — Create display task
- `void Task_Display_Log(const char *msg)` — Append to display log

### wallet_shared.h

- `wallet_tx_t` — Transaction data (recipient, amount, currency)
- `display_context_t` — Display state (IP, MAC, log, etc.)
