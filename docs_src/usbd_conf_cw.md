\page usbd_conf_cw "usbd_conf_cw: USB device BSP (PCD MSP + static buffers)"

# `usbd_conf_cw.c` + `usbd_conf_cw.h`

<brief>Модуль `usbd_conf_cw` — это BSP/конфигурация для USB device middleware: он задаёт статический аллокатор для USBD, описывает memory hooks и настраивает MSP для PCD (GPIO alternate function, enable clock, NVIC priority/enable), а также мостит события HAL_PCD callback’ами в USBD_LL.</brief>

## Краткий обзор
<brief>Модуль `usbd_conf_cw` — это BSP/конфигурация для USB device middleware: он задаёт статический аллокатор для USBD, описывает memory hooks и настраивает MSP для PCD (GPIO alternate function, enable clock, NVIC priority/enable), а также мостит события HAL_PCD callback’ами в USBD_LL.</brief>

## Abstract (Synthèse логики)
USB middleware (USBD) на STM32 требует связки:
- hardware-independent layer (USBD core),
- board/hardware-dependent layer (MSP init for PCD + перевод HAL событий в USBD_LL функции),
- буферные ограничения/аллокаторы.

`usbd_conf_cw` обеспечивает эту “переходную” инфраструктуру. Бизнес-роль — сделать так, чтобы класс WebUSB в `usb_webusb` мог работать поверх корректно сконфигурированного USB peripheral (PCD) и предсказуемого набора memory hooks.

## Logic Flow (MSP init + callback bridging)
Основные блоки:
### MSP init/deinit для PCD
При `HAL_PCD_MspInit(PCD_HandleTypeDef *pcdHandle)`:
1. Проверить, что используется `USB1_OTG_HS` (ранний exit для других).
2. Включить такт GPIOA.
3. Настроить PA11/PA12:
   - AF push-pull,
   - pull: nopull,
   - очень высокая скорость,
   - alternate function `GPIO_AF10_OTG1_FS`.
4. Включить такт `USB1_OTG_HS_CLK_ENABLE()`.
5. Настроить NVIC:
   - priority (6,0),
   - включить IRQ `OTG_HS_IRQn`.

`HAL_PCD_MspDeInit` делает обратное: отключает clock, деинициализирует GPIO, выключает IRQ.

### Bridging callback’ов
Функции `HAL_PCD_*Callback` переводят HAL-эквивалентные события в `USBD_LL_*`:
- Setup stage -> `USBD_LL_SetupStage`,
- DataOut stage -> `USBD_LL_DataOutStage`,
- DataIn stage -> `USBD_LL_DataInStage`,
- SOF/Reset/Suspend/Resume и т.д.

### USBD static memory hooks
Есть `usbd_webusb_mem` массив и функции:
- `USBD_static_malloc(size)` возвращает указатель на статический буфер (без реальной аллокации),
- `USBD_static_free(p)` no-op.

## Прерывания/регистры
Регистр-уровневые операции не производятся напрямую: конфигурация NVIC/GPIO выполняется через HAL.
ISR реализованы в `stm32h7xx_it_usb.c` и зависят от корректной настройки IRQ в MSP init.

## Тайминги
Тайминги задаются middleware-логикой USBD/HAL:
| Элемент | Происхождение |
|---|---|
| NVIC priority | фиксированная константа в MSP init |
| buffer sizes FIFO | определяются в USBD конфигурации (см. PCD init) |

## Dependencies
Прямые:
- STM32 HAL: `HAL_PCD_*`, `HAL_GPIO_Init`, RCC/NVIC
- USBD middleware: `usbd_core.h`, `usbd_ctlreq.h`, `usbd_ioreq.h`
- WebUSB class конфигурации: `usb_webusb.h`.

Глобальные:
- `hpcd_USB_FS` объявлен как external в `usb_device.c` и используется в IRQ обработчике.

## Связи
- `stm32h7xx_it_usb.md` (bridge в IRQ -> HAL_PCD_IRQHandler)
- `usb_device.md` (init запускает middleware)
- `usb_webusb.md` (класс/EP/команды)

