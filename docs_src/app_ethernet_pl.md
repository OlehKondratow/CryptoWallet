\page app_ethernet "app_ethernet: interfejsy Ethernet glue (link callback + DHCP FSM)"

# `Core/Inc/app_ethernet.h`

<brief>Nagłówek `app_ethernet` definiuje interfejsy warstwy Ethernet "glue": deklaruje callback `ethernet_link_status_updated()`, funkcje odczytu bieżących stanów LED i, gdy `LWIP_DHCP` włączony, deklaruje `DHCP_Thread()` i zestaw stałych stanu FSM DHCP używanych przez implementację w `Src/app_ethernet_cw.c`.</brief>

## Przegląd

Nagłówek `app_ethernet` definiuje interfejsy warstwy Ethernet "glue": deklaruje callback `ethernet_link_status_updated()`, funkcje odczytu bieżących stanów LED i, gdy `LWIP_DHCP` włączony, deklaruje `DHCP_Thread()` i zestaw stałych stanu FSM DHCP używanych przez implementację w `Src/app_ethernet_cw.c`.

## Przepływ logiki (kontrakt + model stanu)

Run-time state machine zaimplementowana w `app_ethernet_cw.c`, ale stan sformalizowany tutaj jako stałe:

| Stan | Wartość | Znaczenie |
|------|---------|-----------|
| `DHCP_OFF` | 0 | DHCP nieaktywny |
| `DHCP_START` | 1 | przejście do startu DHCP po link-up |
| `DHCP_WAIT_ADDRESS` | 2 | oczekiwanie na dzierżawę/adres |
| `DHCP_ADDRESS_ASSIGNED` | 3 | adres otrzymany |
| `DHCP_TIMEOUT` | 4 | DHCP nie powiódł się, wybrano fallback statyczny |
| `DHCP_LINK_DOWN` | 5 | link wypadł, DHCP musi się zatrzymać/zresetować |

## Przerwania i rejestry

Nagłówek nie zawiera ISR i rejestrów — tylko deklaracje.

## Zależności

Deklaracja funkcji i stałych stanu:
- `ethernet_link_status_updated()` — wywoływana przez LwIP przy zmianę link
- `DHCP_Thread()` — główna pętla wątku dla FSM DHCP
- Wartości enum stanu używane przez `app_ethernet_cw.c`

## Relacje modułów

- `app_ethernet_cw.md` (implementacja Ethernet glue)
- `task_net.md` (używa link callbacks)
- `task_io.md` (zarządzanie LED na podstawie link state)
