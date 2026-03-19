\page stm32h7xx_hal_msp "stm32h7xx_hal_msp: HAL MspInit for I2C1 + UART3"

# `stm32h7xx_hal_msp.c`

<brief>Файл `stm32h7xx_hal_msp` задаёт “железную” часть для HAL: `HAL_I2C_MspInit` конфигурирует I2C1 (PB8/PB9 под AF OD с подтяжкой), а `HAL_UART_MspInit` настраивает USART3 (TX/RX пины и клоки). Это гарантирует, что `hw_init` сможет инициализировать I2C/I2C-периферию и UART без ручного pin-парсинга в приложении.</brief>

## Краткий обзор
<brief>Файл `stm32h7xx_hal_msp` задаёт “железную” часть для HAL: `HAL_I2C_MspInit` конфигурирует I2C1 (PB8/PB9 под AF OD с подтяжкой), а `HAL_UART_MspInit` настраивает USART3 (TX/RX пины и клоки). Это гарантирует, что `hw_init` сможет инициализировать I2C/I2C-периферию и UART без ручного pin-парсинга в приложении.</brief>

## Abstract (Synthèse логики)
HAL в STM32 разделяет две задачи:
1) “что” инициализировать (параметры периферии) — делается в `hw_init` и HAL init вызовах,
2) “как” именно подключить пины/клоки/IRQ — делается в MSP layer (`stm32h7xx_hal_msp.c`).

Этот файл реализует именно second layer, связывая макросы пинов из `main.h` с соответствующими HAL структурами.

## Logic Flow (MSP init)
### I2C MSP
`HAL_I2C_MspInit(hi2c)`:
1. Если `hi2c->Instance == I2Cx`:
   - включить клок GPIO порта,
   - включить клок I2C,
   - задать SCL/SDA:
     - mode `GPIO_MODE_AF_OD`,
     - pull-up `GPIO_PULLUP`,
     - низкая скорость,
     - alternate function `I2Cx_AF`.

`HAL_I2C_MspDeInit` выполняет reset-подход: force reset/release reset и деинициализация пинов.

### UART MSP
`HAL_UART_MspInit(huart)`:
1. Если `huart->Instance == USARTx`:
   - настроить peripheral clock selection для USART234578,
   - включить клоки GPIO для TX/RX и USART,
   - настроить TX/RX пины:
     - alternate function `USARTx_TX_AF` и `USARTx_RX_AF`,
     - mode AF_PP, pull-up, очень высокая скорость.

`HAL_UART_MspDeInit` делает force reset/release reset и деинициализацию пинов.

## Прерывания/регистры
Файл не реализует ISR. Он работает через HAL GPIO/RCC интерфейсы (внутри HAL могут быть регистровые операции).

## Тайминги
Нет. Это статическая конфигурация перед запуском периферии.

## Dependencies
Прямые:
- Макросы из `main.h`: `I2Cx_*`, `USARTx_*`, clock enable macros
- HAL типы: `I2C_HandleTypeDef`, `UART_HandleTypeDef`

Куда влияет:
- `hw_init.md` / `hw_init.c` вызывает `HAL_I2C_Init` и `HAL_UART_Init`, а MSP обеспечивает корректный pinmux/clock.

## Связи
- `hw_init.md` (кто вызывает HAL init)
- `ssd1306_conf.md` (I2C target для SSD1306)

