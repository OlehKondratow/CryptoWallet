\page usbd_conf_cw "usbd_conf_cw: USB device BSP (PCD MSP + статические буферы)"

# `usbd_conf_cw.c` + `usbd_conf_cw.h`

<brief>Модуль `usbd_conf_cw` — это BSP/конфигурация для USB device middleware: он предоставляет статический аллокатор для USBD, описывает memory hooks, настраивает MSP для PCD (GPIO alternate function, включение clock, NVIC priority/enable), и мостит HAL_PCD callback'ы в USBD_LL события.</brief>

## Обзор

<brief>Модуль `usbd_conf_cw` — это BSP/конфигурация для USB device middleware: он предоставляет статический аллокатор для USBD, описывает memory hooks, настраивает MSP для PCD (GPIO alternate function, включение clock, NVIC priority/enable), и мостит HAL_PCD callback'ы в USBD_LL события.</brief>

## Абстракция (синтез логики)

USB middleware (USBD) на STM32 требует связки между:
- Hardware-independent layer (USBD core)
- Board/hardware-dependent layer (MSP init для PCD + трансляция HAL событий в USBD_LL функции)
- Ограничения буферов/аллокаторы

`usbd_conf_cw` обеспечивает эту "переходную" инфраструктуру. Его бизнес-роль — обеспечить, чтобы класс WebUSB в `usb_webusb` мог работать поверх корректно сконфигурированного USB peripheral (PCD) с предсказуемым набором memory hooks.

## Поток логики (MSP инит + мостирование callback'ов)

Основные блоки:

### MSP Init/Deinit для PCD

В `HAL_PCD_MspInit(PCD_HandleTypeDef *pcdHandle)`:
1. Проверить, что используется `USB1_OTG_HS` (ранний выход для других)
2. Включить clock GPIOA
3. Конфигурировать PA11/PA12:
   - AF push-pull
   - Pull: nopull
   - Очень высокая скорость
   - Alternate function: `GPIO_AF10_OTG1_FS`
4. Включить `USB1_OTG_HS_CLK_ENABLE()`
5. Конфигурировать NVIC:
   - Priority (6,0)
   - Включить IRQ `OTG_HS_IRQn`

`HAL_PCD_MspDeInit` делает обратное: отключает clock, деинициализирует GPIO, выключает IRQ.

### Мостирование callback'ов

Функции `HAL_PCD_*Callback` транслируют HAL-эквивалентные события в `USBD_LL_*`:
- Setup stage → `USBD_LL_SetupStage`
- DataOut stage → `USBD_LL_DataOutStage`
- DataIn stage → `USBD_LL_DataInStage`
- SOF/Reset/Suspend/Resume и т.д.

### USBD статические memory hooks

Есть `usbd_webusb_mem` массив и функции:
- `USBD_static_malloc(size)` возвращает указатель на статический буфер (без реальной аллокации)
- `USBD_static_free(p)` — no-op

## Прерывания/регистры

Регистр-уровневые операции не производятся напрямую: конфигурация NVIC/GPIO выполняется через HAL.
ISR реализованы в `stm32h7xx_it_usb.c` и зависят от корректной настройки IRQ в MSP init.

## Времена

Времена определяются middleware-логикой USBD/HAL:

| Элемент | Происхождение |
|---|---|
| NVIC priority | Фиксированная константа в MSP init |
| Размеры FIFO буферов | Определены в конфигурации USBD (см. PCD init) |

## Зависимости

Прямые:
- STM32 HAL: `HAL_PCD_*`, `HAL_GPIO_Init`, RCC/NVIC
- USBD middleware: `usbd_core.h`, `usbd_ctlreq.h`, `usbd_ioreq.h`
- Конфигурация WebUSB класса: `usb_webusb.h`

Глобальные:
- `hpcd_USB_FS` объявлен как external в `usb_device.c` и используется в IRQ обработчике

## Связи

- `stm32h7xx_it_usb.md` — Мост от IRQ к HAL_PCD_IRQHandler
- `usb_device.md` — Инициализация middleware
- `usb_webusb.md` — Класс/EP/команды
