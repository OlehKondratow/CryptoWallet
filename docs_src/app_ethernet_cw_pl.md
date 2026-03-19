\page app_ethernet_cw "app_ethernet_cw: Ethernet link FSM i DHCP state machine"
\related ethernet_link_status_updated
\related DHCP_Thread
\related ethernet_set_led

# `app_ethernet_cw.c`

<brief>Moduł `app_ethernet_cw` zapewnia obsługę Ethernet: FSM dla stanów link-up/link-down, klient DHCP (START → WAIT_ADDRESS → ASSIGNED/TIMEOUT), informacja zwrotna LED (LED2 = sieć, LED3 = link-down), rejestrowanie adresu IP.</brief>

## Przegląd

Moduł `app_ethernet_cw` zapewnia obsługę Ethernet: FSM dla stanów link-up/link-down, klient DHCP (START → WAIT_ADDRESS → ASSIGNED/TIMEOUT), informacja zwrotna LED (LED2 = sieć, LED3 = link-down), rejestrowanie adresu IP.

## Przepływ logiki (Link + DHCP FSM)

### Link-up callback

`ethernet_link_status_updated(netif)` wywoływana gdy kabel podłączony/odłączony:
1. Jeśli link UP i był DOWN:
   - Gdy `LWIP_DHCP=1`: przechodzi do `DHCP_START` (wątek DHCP się zaczyna)
   - Gdy `LWIP_DHCP=0`: natychmiast włącza LED2, uruchamia `time_service_start()`
2. Jeśli link DOWN:
   - Gdy `LWIP_DHCP=1`: przechodzi do `DHCP_LINK_DOWN`
   - Gdy `LWIP_DHCP=0`: włącza LED3 (ostrzeżenie)

### DHCP FSM (gdy `LWIP_DHCP=1`)

`DHCP_Thread(netif)` pracuje jako maszyna stanów:
1. **DHCP_START:** zeruje IP/mask/gw, przechodzi do WAIT_ADDRESS, wyłącza oba LED, wywołuje `netifapi_dhcp_start()`
2. **DHCP_WAIT_ADDRESS:** odpytuje `dhcp_supplied_address()`:
   - Jeśli IP otrzymany: rejestruje adres, włącza LED2, uruchamia `time_service_start()`, przechodzi do ASSIGNED
   - Jeśli przekroczony licznik ponowień (>12): fallback na statyczny IP z `main.h`, rejestruje, włącza LED2, przechodzi do TIMEOUT
3. **DHCP_LINK_DOWN:** wyłącza DHCP, włącza LED3
4. Powtarza co 500 ms

## Zależności

Bezpośrednie:
- Mechanizm callback netif LwIP
- Funkcje klienta DHCP: `netifapi_dhcp_start()`, `dhcp_supplied_address()`
- Kontrola LED: zapisy GPIO LED2/LED3
- Time service: `time_service_start()` przy przypisaniu IP

Pośrednie:
- `app_ethernet.h` (deklaruje interfejsy)
- `task_net.md` (używa łączności IP)
- `task_io.md` (zarządzanie LED)

## Relacje modułów

- `app_ethernet.md` (deklaruje interfejsy)
- `task_net.md` (serwer HTTP zależy od linku)
- `task_io.md` (polityka LED)
- `time_service.md` (synchronizacja NTP po DHCP)
