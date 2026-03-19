\page stm32h7xx_hal_msp "stm32h7xx_hal_msp: HAL MspInit для I2C1 + UART3"

# `stm32h7xx_hal_msp.c`

<brief>Файл `stm32h7xx_hal_msp` задаёт "железную" часть для HAL: `HAL_I2C_MspInit` конфигурирует I2C1 (PB8/PB9 как AF OD с подтяжкой), а `HAL_UART_MspInit` настраивает USART3 (TX/RX пины и клоки). Это гарантирует, что `hw_init` может инициализировать I2C периферию и UART без ручной конфигурации пинов в приложении.</brief>

## Обзор

<brief>Файл `stm32h7xx_hal_msp` задаёт "железную" часть для HAL: `HAL_I2C_MspInit` конфигурирует I2C1 (PB8/PB9 как AF OD с подтяжкой), а `HAL_UART_MspInit` настраивает USART3 (TX/RX пины и клоки). Это гарантирует, что `hw_init` может инициализировать I2C периферию и UART без ручной конфигурации пинов в приложении.</brief>

## Абстракция (синтез логики)

HAL в STM32 разделяет две задачи:
1) "Что" инициализировать (параметры периферии) — делается в `hw_init` и HAL init вызовах
2) "Как" подключить пины/клоки/IRQ — делается в MSP layer (`stm32h7xx_hal_msp.c`)

Этот файл реализует второй слой, связывая макросы пинов из `main.h` с соответствующими HAL структурами.

## Поток логики (MSP инициализация)

### I2C MSP

`HAL_I2C_MspInit(hi2c)`:
1. Если `hi2c->Instance == I2Cx`:
   - Включить клок GPIO порта
   - Включить клок I2C
   - Конфигурировать SCL/SDA:
     - Режим: `GPIO_MODE_AF_OD`
     - Подтяжка: `GPIO_PULLUP`
     - Скорость: Низкая
     - Альтернативная функция: `I2Cx_AF`

`HAL_I2C_MspDeInit` использует force-reset подход: reset/release и деинициализация пинов.

### UART MSP

`HAL_UART_MspInit(huart)`:
1. Если `huart->Instance == USARTx`:
   - Конфигурировать выбор периферального клока для USART234578
   - Включить клоки для TX/RX GPIO и USART
   - Конфигурировать TX/RX пины:
     - Альтернативные функции: `USARTx_TX_AF` и `USARTx_RX_AF`
     - Режим: AF_PP, подтяжка, очень высокая скорость

`HAL_UART_MspDeInit` выполняет force-reset/release и деинициализацию пинов.

## Прерывания/регистры

Файл не реализует ISR. Работает через HAL GPIO/RCC интерфейсы (регистровые операции внутри HAL).

## Времена

Нет. Это статическая конфигурация перед запуском периферии.

## Зависимости

Прямые:
- Макросы из `main.h`: `I2Cx_*`, `USARTx_*`, макросы включения клока
- HAL типы: `I2C_HandleTypeDef`, `UART_HandleTypeDef`

Влияние вниз по потоку:
- `hw_init.md` / `hw_init.c` вызывает `HAL_I2C_Init` и `HAL_UART_Init`; MSP обеспечивает правильный pinmux/clock

## Связи

- `hw_init.md` — Вызывающая сторона HAL init функций
- `ssd1306_conf.md` — I2C цель для дисплея SSD1306
