\page task_io "task_io: polityka LED (heartbeat + security alert)"
\related Task_IO_Create

# `task_io.c` + `task_io.h`

<brief>Moduł `task_io` zarządza wizualnymi wskaźnikami bezpieczeństwa i stanu systemu: okresowo aktualizuje LED1 jako "alive", zarządza LED2 jako wskaźnikiem sieciowym (w zależności od kompilacji LwIP) i włącza LED3 w przypadku security alert.</brief>

## Przegląd

Moduł `task_io` zarządza wizualnymi wskaźnikami bezpieczeństwa i stanu systemu: okresowo aktualizuje LED1 jako "alive", zarządza LED2 jako wskaźnikiem sieciowym (w zależności od kompilacji LwIP) i włącza LED3 w przypadku security alert.

## Przepływ logiki (prosty control loop)

Główna pętla:
1. Raz po starcie, włącza LED1 i wyłącza LED3
2. Następnie co `POLL_MS=100ms`:
   - LED1 wymuszony w stanie "ON" (heartbeat)
   - LED2 okresowo przełączany tylko w kompilacji bez LwIP (`!USE_LWIP`)
   - LED3 ustawiony warunkiem `g_security_alert != 0`

Warunkowa logika LED2:

| Warunek kompilacji | Zachowanie LED2 |
|---|---|
| `USE_LWIP=1` | LED2 nie przełączany (w bieżącej logice — zawsze false po init) |
| `USE_LWIP=0` | LED2 miga z okresem zależnym od licznika tiksów (`tick_count/25`) |

## Przerwania i rejestry

Brak bezpośredniego ISR i pracy rejestrów. Używa zapisów GPIO HAL:
- `HAL_GPIO_WritePin(LED*_GPIO_PORT, LED*_PIN, level)`

## Czasy i warunki rozgałęzienia

| Parametr | Wartość |
|----------|---------|
| okres pętli | `POLL_MS=100ms` |
| stos/priorytet | `IO_STACK_SIZE=128`, `IO_PRIORITY = idle+1` |
| źródło stanu LED3 | `g_security_alert` |

## Zależności

Bezpośrednie zależności:
- Flaga globalna: `g_security_alert` (z `task_sign` lub `task_security`)
- Definiuje GPIO: `LED1_GPIO_PORT/PIN`, `LED2_GPIO_PORT/PIN`, `LED3_GPIO_PORT/PIN` (z `main.h`)
- HAL: `stm32h7xx_hal.h` dla `HAL_GPIO_WritePin()`
- FreeRTOS: `vTaskDelay()`, tworzenie zadania

## Relacje modułów

- `task_sign.md` (źródło `g_security_alert` na błędy)
- `task_security.md` (alternatywne źródło security state)
- `hw_init.md` (GPIO init dla LED)
