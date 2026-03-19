\page stm32h7xx_it "stm32h7xx_it: Most SysTick + ETH IRQ"

# `stm32h7xx_it.c`

<brief>Plik `stm32h7xx_it` implementuje krytyczne procedury obsługi przerwań dla głównego scenariusza: `SysTick_Handler` łączy zegar HAL z zegarem FreeRTOS, a gdy LwIP jest włączony, obsługuje IRQ Ethernet za pośrednictwem `HAL_ETH_IRQHandler`.</brief>

## Przegląd

<brief>Plik `stm32h7xx_it` implementuje krytyczne procedury obsługi przerwań dla głównego scenariusza: `SysTick_Handler` łączy zegar HAL z zegarem FreeRTOS, a gdy LwIP jest włączony, obsługuje IRQ Ethernet za pośrednictwem `HAL_ETH_IRQHandler`.</brief>

## Abstrakcja (synteza logiki)

W osadzonym projekcie FreeRTOS, prawidłowe działanie opóźnień i harmonogramu wymaga, aby SysTick jednocześnie aktualizował:
- "systemowy" zegar HAL (potrzebny dla `HAL_Delay`, wymagany dla I2C/OLED i innych podsystemów opartych na HAL)
- zegar harmonogramu FreeRTOS (do przełączania i timeoutów)

Ponadto, w sieciowych kompilacjach potrzebny jest IRQ Ethernet do wysyłania obsługi ramek/zdarzeń do LwIP/HAL.

## Przepływ logiki (zachowanie ISR)

Logika ISR nie zawiera automatu stanów, ale ma sekwencję:
1. `SysTick_Handler()`:
   - `HAL_IncTick()` — Aktualizuj licznik taktów HAL
   - `xPortSysTickHandler()` — Powiadom FreeRTOS
2. `ETH_IRQHandler()` (tylko jeśli `USE_LWIP`):
   - `HAL_ETH_IRQHandler(&EthHandle)`

## Przerwania/rejestry

To są pliki procedur obsługi przerwań:
- Bezpośredni dostęp do rejestru nie jest wykonywany; wszystko delegowane do HAL
- Główna "logika rejestru" to wywoływanie funkcji w odpowiedniej kolejności, aby takty HAL i RTOS pozostały spójne

## Czasy i warunki gałęzi

| Warunek | Efekt |
|---|---|
| `USE_LWIP` zdefiniowany | `ETH_IRQHandler` skompilowany |
| SysTick | Zawsze aktywny (dla FreeRTOS i HAL_DELAY) |

## Zależności

Bezpośrednie:
- FreeRTOS: `xPortSysTickHandler()`
- HAL: `HAL_IncTick()`, `HAL_ETH_IRQHandler`, `EthHandle` (zewnętrzny)
- Flagi czasu kompilacji: `USE_LWIP`

## Relacje

- `stm32h7xx_it_systick.md` — Alternatywny program obsługi SysTick dla kompilacji minimal-lwip
- `task_display` używa opóźnień HAL, więc zegar HAL jest obowiązkowy
