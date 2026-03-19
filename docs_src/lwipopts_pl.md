\page lwipopts "lwipopts: LwIP compile-time options (DHCP + SNTP)"

# `Core/Inc/lwipopts.h`

<brief>Nagłówek `lwipopts` definiuje opcje konfiguracji czasu kompilacji dla LwIP: włącza IPv4/TCP/DHCP/DNS/SNTP, konfiguruje rozmiar stosu (`LWIP_RAM_HEAP_POINTER` i `MEM_SIZE`), parametry bufora TCP/okna, wiąże aktualizacje SNTP z `time_service_set_epoch()`.</brief>

## Przegląd

<brief>Nagłówek `lwipopts` definiuje opcje konfiguracji czasu kompilacji dla LwIP: włącza IPv4/TCP/DHCP/DNS/SNTP, konfiguruje rozmiar stosu (`LWIP_RAM_HEAP_POINTER` i `MEM_SIZE`), parametry bufora TCP/okna, wiąże aktualizacje SNTP z `time_service_set_epoch()`.</brief>

## Abstrakcja (synteza logiki)

Osadzony stos sieciowy to nie tylko "jakie pakiety są wysyłane", ale także "jakie zasoby i struktury są dostępne". `lwipopts.h` ustala te zasoby i zachowania:

- Które podsystemy LwIP są kompilowane
- Gdzie stos znajduje się w pamięci
- Jak strukturowane są bufory PBUF i okna TCP
- Jak SNTP integruje się z aplikacją poprzez `SNTP_SET_SYSTEM_TIME(sec)`

Cel biznesowy pliku config: pogodzić LwIP z ograniczeniami STM32H7 (pamięć D2 SRAM i interakcja pamięci podręcznej/MPU) i z aplikacją `time_service`.

## Przepływ logiki (compile-time "automat stanów")

To nie jest runtime automat stanów. Logika jest w zestawach makr definiujących zachowanie podsystemu:

| Grupa parametrów | Klucze makra | Cel |
|---|---|---|
| Logowanie alive UART | `LWIP_ALIVE_LOG` | Drukuj "alive" okresowo w ścieżce LwIP |
| Stos/Pamięć | `MEM_ALIGNMENT`, `MEM_SIZE`, `LWIP_RAM_HEAP_POINTER` | Rozmiar i umiejscowienie stosu |
| Threading | `TCPIP_THREAD_*`, `DEFAULT_THREAD_STACKSIZE` | Parametry wewnętrznych wątków TCP/IP |
| Włączenie protokołów | `LWIP_IPV4`, `LWIP_TCP`, `LWIP_DHCP`, `LWIP_SNTP`, `LWIP_DNS` | Które podsystemy kompilują |
| Integracja czasu | `SNTP_SET_SYSTEM_TIME(sec)` | Wywołuje `time_service_set_epoch(sec)` |
| Callback łącza | `LWIP_NETIF_LINK_CALLBACK` (przez `LWIP_NO_LINK_THREAD`) | Jak LwIP raportuje zmiany łącza |

## Przerwania/rejestry

Nie. To konfiguracja czasu kompilacji.

## Czasy i warunki krytyczne

| Parametr | Wartość | Kontekst |
|---|---|---|
| `SNTP_UPDATE_DELAY` | `15*60*1000` ms | Okres aktualizacji czasu |
| `MEM_SIZE` | `14*1024` bajtów | Wpływa na zdolność LwIP do obsługi połączeń i alokacji buforu |
| Bufor TCP/okno | `TCP_SND_BUF`, `TCP_WND` | Wpływa na przepustowość |

## Zależności

Bezpośrednie:
- `time_service_set_epoch()` z `time_service.h`

Pośrednio:
- `hw_init` dla prawidłowego MPU/cache w gałęzi `USE_LWIP` (stos LwIP umieszczony na `0x30004000` jako część konfiguracji MPU)

## Relacje

- `hw_init.md` — Dlaczego stos/cache muszą zgadzać się z LwIP
- `time_service.md` — Obsługa epoki SNTP przez `time_service_set_epoch`
- `task_net.md` / `app_ethernet_cw.md` — Runtime logika DHCP i uruchamianie SNTP
