\page stm32h7xx_it_systick "stm32h7xx_it_systick: SysTick dla minimal-lwip"

# `stm32h7xx_it_systick.c`

<brief>Plik `stm32h7xx_it_systick` rozwiązuje sytuację dla kompilacji minimal-lwip: udostępnia `SysTick_Handler`, który synchronizuje zegar HAL i zegar FreeRTOS, aby uniknąć zawieszenia/nieprawidłowego zachowania, gdy podstawowy `stm32h7xx_it.c` nie zawiera `SysTick_Handler`.</brief>

## Przegląd

<brief>Plik `stm32h7xx_it_systick` rozwiązuje sytuację dla kompilacji minimal-lwip: udostępnia `SysTick_Handler`, który synchronizuje zegar HAL i zegar FreeRTOS, aby uniknąć zawieszenia/nieprawidłowego zachowania, gdy podstawowy `stm32h7xx_it.c` nie zawiera `SysTick_Handler`.</brief>

## Abstrakcja (synteza logiki)

SysTick w STM32 to krytyczne przerwanie: zasilone zarówno HAL timing jak i FreeRTOS scheduling. W niektórych konfiguracjach minimalnego LwIP (jak wspomniano w komentarzach pliku) podstawowy `stm32h7xx_it.c` może nie podłączyć poprawnej procedury obsługi, co prowadzi do niebezpiecznych konsekwencji (aż do nieskończonej pętli backtrace / WWDG).

`stm32h7xx_it_systick` to docelowe naprawienie, zapewniające, że SysTick jest zawsze obsługiwany prawidłowymi wywołaniami.

## Przepływ logiki (ISR)

W ISR:
1. `HAL_IncTick()` — Aktualizuj zegar HAL (wymagane dla `HAL_Delay`)
2. `xPortSysTickHandler()` — Powiadom FreeRTOS o takcie

## Przerwania/rejestry

To jest plik ISR:
- Przetwarzanie odbywa się w procedurze obsługi przerwania SysTick
- Rejestry nie są bezpośrednio manipulowane; używane są hooki HAL/FreeRTOS

## Czasy

Wywołania powinny być szybkie, w przeciwnym razie harmonogram zostanie zakłócony:
- ISR ograniczone do dwóch wywołań, bez pętli/parsowania

## Zależności

- HAL: `HAL_IncTick`
- FreeRTOS: `xPortSysTickHandler`

## Relacje

- `stm32h7xx_it.md` — Podstawowa implementacja procedur w kompilacji "pełnej"
