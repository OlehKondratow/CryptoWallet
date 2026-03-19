\page task_user "task_user: polityka przycisku USER (potwierdzenie/odrzucenie)"
\related Task_User_Create

# `task_user.c` + `task_user.h`

<brief>Moduł `task_user` implementuje fizyczną logikę UX dla przycisku USER (PC13): wykonuje debounce, rozróżnia krótkie naciśnięcie (Confirm) od długiego przytrzymania (~2.5s) jako Reject i sygnalizuje to do `task_sign` poprzez `g_user_event_group`.</brief>

## Przegląd

Moduł `task_user` implementuje fizyczną logikę UX dla przycisku USER (PC13): wykonuje debounce, rozróżnia krótkie naciśnięcie (Confirm) od długiego przytrzymania (~2.5s) jako Reject i sygnalizuje to do `task_sign` poprzez `g_user_event_group`.

## Przepływ logiki (button state machine)

Podejście: okresowe sondowanie z filtrowaniem stabilności.

Wyzwalacze i progi:

| Parametr | Wartość |
|----------|---------|
| `POLL_MS` | 20ms |
| Debounce | 50ms |
| Long press | 2500ms |

Stany (niejawnie, poprzez zmienne):
1. Idle (przycisk nie naciśnięty)
2. Pressed-Stabilizing (przycisk naciśnięty, gromadzenie stabilnych tiksów)
3. Confirm-Fired (krótkie naciśnięcie potwierdzone na puszczeniu)
4. Reject-Fired (długie przytrzymanie odrzuca natychmiast podczas przytrzymywania)

Reguły:
1. Podczas gdy przycisk pozostaje naciśnięty — gromadzi `btn_stable_ticks`
2. Jeśli przytrzymanie osiągnie `LONG_PRESS_MS` i akcja jeszcze nie "fired" — ustawia `EVENT_USER_REJECTED`
3. Na puszczeniu:
   - jeśli czas trwania w zakresie `[DEBOUNCE_MS, LONG_PRESS_MS)` i akcja jeszcze nie "fired" — ustawia `EVENT_USER_CONFIRMED`
   - następnie resetuje flagi czasu trwania

## Przerwania i rejestry

Brak bezpośredniego ISR: moduł pracuje jako zadanie FreeRTOS i czyta stan GPIO poprzez HAL.
Dokładność timing na poziomie `POLL_MS` (20ms): zdarzenie może przesunąć się co najwyżej o jeden tik.

## Czasy i warunki rozgałęzienia

| Rozgałęzienie | Warunek | Wynik |
|---|---|---|
| Reject | `last_press_duration >= 2500 && !action_fired` | set `EVENT_USER_REJECTED` |
| Confirm | `DEBOUNCE_MS <= last_press_duration < 2500 && !action_fired` na puszczeniu | set `EVENT_USER_CONFIRMED` |
| Debounce | podczas gdy czas nie osiągnął 50ms | brak ustawienia zdarzeń |

## Zależności

Bezpośrednie:
- Wejście: `USER_KEY_GPIO_PORT`, `USER_KEY_PIN`, `USER_KEY_PRESSED` (z `main.h`)
- Zdarzenia: `g_user_event_group` + bity `EVENT_USER_CONFIRMED`, `EVENT_USER_REJECTED` (z `wallet_shared.h` / architektury zadań)
- Logowanie: `Task_Display_Log`

Pośrednie:
- `task_sign` czeka te bity zdarzeń i przechodzi system do scenariusza potwierdzenia/odrzucenia

## Relacje modułów

- `task_sign.md` (konsument `g_user_event_group`)
- `task_io.md` (wskaźnik alertu bezpieczeństwa po decyzji)
- `task_display.md` (etykiety "Confirm/Reject" poprzez warstwę logowania)
