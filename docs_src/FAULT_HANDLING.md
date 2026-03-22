# Обработка fault и фатальных ошибок (STM32H7)

## Где что реализовано

| Компонент | Файл |
|-----------|------|
| Дамп Cortex-M (CFSR, HFSR, MMFAR, BFAR, стековый кадр) | `Core/Src/fault_report.c` |
| Включение Mem/Bus/Usage fault, DIV_0_TRP | `Fault_Report_Init()` — вызывается из `HW_Init()` |
| Ethernet IRQ (minimal-lwip) | `Core/Src/stm32h7xx_it_lwip.c` |
| SysTick (HAL + FreeRTOS) | `Core/Src/stm32h7xx_it_systick.c` |
| Фатальные ошибки HAL / приложения | `Error_Handler()`, `Error_Handler_At()`, макрос `ERROR_HALT()` в `main.h` |
| FreeRTOS `configASSERT` | `Fault_ConfigAssertFailed()` — UART **без** mutex |

## UART: обычный лог vs fault

- **`UART_Log` / `APP_LOG_*`**: при работающем планировщике используется мьютекс, чтобы строки не перемешивались.
- **Fault path (`fault_uart_line_isr`, `Fault_ConfigAssertFailed`)**: только прямой `HAL_UART_Transmit` на `huart3`, **без** FreeRTOS API и **без** mutex — иначе deadlock или повреждение состояния в контексте исключения.

## Чтение дампа

После строки `[ERR] [FAULT] kind=...` смотрите:

- **PC** — адрес инструкции, связанной с fault (для отладчика / `addr2line`).
- **CFSR / HFSR** — причина (см. ARM Cortex-M7 Devices Generic User Guide, раздел про CFSR).
- **BFAR / MMFAR** — адрес при bus/memory fault, если соответствующие флаги valid в CFSR.

## Сборка

Цель `minimal-lwip` линкует `fault_report.o` и `stm32h7xx_it_lwip.o` из каталога CryptoWallet (`Makefile`), а не пустые fault-циклы из `lwip_zero/cube_project`.
