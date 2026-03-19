\page usbd_conf_cw "usbd_conf_cw: USB device BSP (PCD MSP + statyczne bufory)"

# `usbd_conf_cw.c` + `usbd_conf_cw.h`

<brief>Moduł `usbd_conf_cw` jest warstwą BSP/konfiguracyjną dla oprogramowania pośredniego USB device: zapewnia statyczny alokator dla USBD, opisuje hooki pamięci, konfiguruje MSP dla PCD (alternatywna funkcja GPIO, włączenie zegara, priorytet/włączenie NVIC) i łączy callback'i HAL_PCD ze zdarzeniami USBD_LL.</brief>

## Przegląd

<brief>Moduł `usbd_conf_cw` jest warstwą BSP/konfiguracyjną dla oprogramowania pośredniego USB device: zapewnia statyczny alokator dla USBD, opisuje hooki pamięci, konfiguruje MSP dla PCD (alternatywna funkcja GPIO, włączenie zegara, priorytet/włączenie NVIC) i łączy callback'i HAL_PCD ze zdarzeniami USBD_LL.</brief>

## Abstrakcja (synteza logiki)

Oprogramowanie pośrednie USB (USBD) na STM32 wymaga powiązania między:
- Warstwą niezależną od sprzętu (rdzeń USBD)
- Warstwą zależną od płyty/sprzętu (inicjalizacja MSP dla PCD + tłumaczenie zdarzeń HAL na funkcje USBD_LL)
- Ograniczenia bufora/alokatora

`usbd_conf_cw` zapewnia tę infrastrukturę "przejściową". Jego rola biznesowa to zapewnienie, że klasa WebUSB w `usb_webusb` może działać na poprawnie skonfigurowanej peryferalnej USB (PCD) ze przewidywalnym zestawem hooków pamięci.

## Przepływ logiki (inicjalizacja MSP + łączenie callback'ów)

Główne bloki:

### Inicjalizacja/deinicjalizacja MSP dla PCD

W `HAL_PCD_MspInit(PCD_HandleTypeDef *pcdHandle)`:
1. Sprawdź, czy użytkowania jest `USB1_OTG_HS` (wcześniejszy wyjście dla innych)
2. Włącz zegar GPIOA
3. Skonfiguruj PA11/PA12:
   - AF push-pull
   - Pull: nopull
   - Bardzo wysoka prędkość
   - Alternatywna funkcja: `GPIO_AF10_OTG1_FS`
4. Włącz `USB1_OTG_HS_CLK_ENABLE()`
5. Skonfiguruj NVIC:
   - Priorytet (6,0)
   - Włącz IRQ `OTG_HS_IRQn`

`HAL_PCD_MspDeInit` robi odwrotnie: wyłącza zegar, deinicjalizuje GPIO, wyłącza IRQ.

### Łączenie callback'ów

Funkcje `HAL_PCD_*Callback` tłumaczą zdarzenia równoważne HAL na `USBD_LL_*`:
- Setup stage → `USBD_LL_SetupStage`
- DataOut stage → `USBD_LL_DataOutStage`
- DataIn stage → `USBD_LL_DataInStage`
- SOF/Reset/Suspend/Resume itp.

### Hooki statycznej pamięci USBD

Istnieje tablica `usbd_webusb_mem` i funkcje:
- `USBD_static_malloc(size)` zwraca wskaźnik do bufora statycznego (bez rzeczywistej alokacji)
- `USBD_static_free(p)` jest no-op

## Przerwania/rejestry

Operacje na poziomie rejestru nie są wykonywane bezpośrednio: konfiguracja NVIC/GPIO jest wykonywana poprzez HAL.
ISR są implementowane w `stm32h7xx_it_usb.c` i zależą od prawidłowej konfiguracji IRQ w inicjalizacji MSP.

## Czasy

Czasy są określone przez logikę oprogramowania pośredniego USBD/HAL:

| Element | Pochodzenie |
|---|---|
| Priorytet NVIC | Stała Fixed w inicjalizacji MSP |
| Rozmiary bufora FIFO | Zdefiniowane w konfiguracji USBD (patrz PCD init) |

## Zależności

Bezpośrednie:
- STM32 HAL: `HAL_PCD_*`, `HAL_GPIO_Init`, RCC/NVIC
- Oprogramowanie pośrednie USBD: `usbd_core.h`, `usbd_ctlreq.h`, `usbd_ioreq.h`
- Konfiguracja klasy WebUSB: `usb_webusb.h`

Globalnie:
- `hpcd_USB_FS` zadeklarowany jako zewnętrzny w `usb_device.c` i używany w procedurze obsługi IRQ

## Relacje

- `stm32h7xx_it_usb.md` — Most od IRQ do HAL_PCD_IRQHandler
- `usb_device.md` — Inicjalizacja oprogramowania pośredniego
- `usb_webusb.md` — Klasa/EP/polecenia
