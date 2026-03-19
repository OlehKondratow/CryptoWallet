\page hw_init "hw_init: board bring-up (clock/MPU/GPIO/I2C/UART/USB)"
\related HW_Init
\related HW_Init_Early_LwIP
\related UART_Log

# `hw_init.c` + `hw_init.h`

<brief>Модуль `hw_init` отвечает за низкоуровневый bring-up платы: он формирует правильный порядок тактов/кэша/MPU (критично для LwIP/ETH), инициализирует GPIO для UX (LED/кнопка), поднимает I2C1 под SSD1306 и UART-лог, а также опционально поднимает USB и RNG.</brief>

## Краткий обзор
<brief>Модуль `hw_init` отвечает за низкоуровневый bring-up платы: он формирует правильный порядок тактов/кэша/MPU (критично для LwIP/ETH), инициализирует GPIO для UX (LED/кнопка), поднимает I2C1 под SSD1306 и UART-лог, а также опционально поднимает USB и RNG.</brief>

## Abstract (Synthèse логики)
`hw_init` — это "слой доверия" между HAL/Cube и приложением: он гарантирует, что до любой сетевой логики (если включена `USE_LWIP`) MPU и кэш настроены корректно для DMA и областей памяти, а затем что периферия, нужная задачам (I2C/OLED, UART и базовые GPIO), доступна в ожидаемом состоянии.

## Logic Flow (bootstrap flow)
С точки зрения порядка операций важны две точки входа:
1. `HW_Init_Early_LwIP()` выполняется до `HAL_Init()` и включается только при `USE_LWIP`.
2. `HW_Init()` выполняется после `HAL_Init()` и отвечает за основной bring-up: часы, GPIO, I2C1, UART3 и опциональные USB/RNG; инициализация OLED-части зависит от `SKIP_OLED`.

## Детальный анализ

### Назначение и "границы" ответственности
`hw_init` — это оболочка над HAL-инитами. По дизайну:
- `main.c` готовит IPC/таски и стартует FreeRTOS, а также вызывает функции init в нужной последовательности.
- `hw_init.c` держит "железный" порядок: часы, кэш/MPU, периферия (GPIO/I2C/UART) и зависимости по флагам сборки.
- Сетевая логика (Ethernet PHY/DHCP/линк) не находится здесь напрямую: она "включается" окружением LwIP/Cube и реализована в других модулях (например, через Cube/BSP + `Src/app_ethernet_cw.c`).

### Точка входа №1: `HW_Init_Early_LwIP()` (только при `USE_LWIP`)
Эта функция вызывается из `main.c` **до** `HAL_Init()` и нужна для того, чтобы Cortex-M7 корректно работал с DMA/областями памяти, которые использует Ethernet/LwIP.

Ключевая логика внутри:
1. `HAL_MPU_Disable()` — чтобы безопасно перенастроить регионы.
2. Формирование трёх MPU-регионов:
   - **Region 0**: "закрыть всё" (4GB no-access) как дефолт.
   - **Region 1**: область **`0x30000000`** (1 KB) под **ETH descriptors** (дескрипторы DMA).
   - **Region 2**: область **`0x30004000`** (16 KB) под **LwIP heap**.
3. `HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT)` — включение MPU.
4. Включение кэшей: `SCB_EnableICache()` и `SCB_EnableDCache()`.

Почему это важно для DMA:
- Ethernet DMA читает/пишет в память, и если адреса дескрипторов/кучи не настроены так, кэш может "перехватить" данные и привести к рассинхронизации.
- Конкретные базы/размеры подобраны под используемый LwIP/Cube-пайплайн (шаблон `lwip_zero`).

### Точка входа №2: `HW_Init()` (после `HAL_Init()`)
`HW_Init()` — "основной" bring-up после `HAL_Init()`:

1. **Сборка без LwIP (`!USE_LWIP`)**
   - вызывается `MPU_Config()` (в текущей реализации — фактически `HAL_MPU_Disable()`; чтобы не держать MPU активным без надобности).
   - `CPU_CACHE_Enable()` выполняется позже уже после смены тактов (как рекомендовано для H7).

2. **Задержка после включения** (пустой цикл ~2M итераций)
   - комментарий указывает на стабилизацию MCO/ST-Link clocking на Nucleo.

3. **`SystemClock_Config()`**
   - включает питание (PWR regulator scale1),
   - включает такт **D2 SRAM3**: `__HAL_RCC_D2SRAM3_CLK_ENABLE()`.
   - настраивает PLL от HSE и задаёт делители AHB/APB. Это обеспечивает корректные частоты для HAL и периферии.

4. `SystemCoreClockUpdate()`
   - синхронизирует переменные HAL с фактическими частотами после переключения SYSCLK.

5. Инициализация периферии:
   - `MX_GPIO_Init()`:
     - включает такты портов LED1/LED2/LED3 и кнопки `USER`,
     - настраивает светодиоды как `GPIO_MODE_OUTPUT_PP`, низкую скорость и выставляет "выкл" уровни,
     - кнопку как `GPIO_MODE_INPUT`.
   - `MX_I2C1_Init()`:
     - настраивает `hi2c1` (Timing ~ "как в соседнем проекте", 7-bit addressing),
     - включает аналоговый фильтр, отключает цифровой,
     - затем `ssd1306_Init()` вызывается сразу после `MX_I2C1_Init()` только если `USE_LWIP && !SKIP_OLED`.
   - `MX_USART3_Init()`:
     - `USARTx` на 115200, 8N1, TX/RX режим,
     - без hardware flow control, oversampling 16.

6. Опциональные ветки:
   - `USE_WEBUSB`:
     - лог в UART через `Task_Display_Log()`,
     - `MX_USB_Device_Init()`,
     - второй лог "USB ready".
   - `USE_CRYPTO_SIGN`:
     - `MX_RNG_Init()` — инициализация аппаратного RNG через HAL (`hrng` + `HAL_RNG_Init()`),
     - также предусмотрен `HAL_RNG_MspInit()` для включения такта `__HAL_RCC_RNG_CLK_ENABLE()`.

### Взаимодействие с `main.c` и другими модулями
В `Core/Src/main.c` порядок такой:
1. `HW_Init_Early_LwIP()` (только при `USE_LWIP`)
2. `HAL_Init()`
3. `HW_Init()`
Далее создаются очереди/семафоры и стартуют таски (`Task_Display_Create`, `Task_Net_Create`, `Task_Sign_Create`, `Task_IO_Create`, `Task_User_Create`).

Практический смысл порядка:
- Ветка LwIP требует MPU/кэш до HAL, чтобы Ethernet DMA и LwIP heap работали корректно.
- Ветка "OLED init" привязана к I2C: поэтому `ssd1306_Init()` выполняется сразу после `MX_I2C1_Init()`.
- UART и USB/RNG (по флагам) подготавливаются в `HW_Init()`, чтобы ранние логи и крипто-база были доступны до основной логики задач.

### Уровень "регистров/параметров" без переписывания Cube
`hw_init` не пытается заменить Cube: он делает точечные HAL-настройки:
- часы/PLL и системные делители через `RCC_*` и `HAL_RCC_*`,
- MPU через `HAL_MPU_ConfigRegion`,
- периферия через HAL `HAL_GPIO_Init`, `HAL_I2C_Init`, `HAL_UART_Init`.
Детали pinmux и MSP-инициализация для I2C/USART обычно "живут" в связке `stm32h7xx_hal_msp.c` (например, для I2C1 и USART3).

## Прерывания/регистры
`hw_init` — единственное место, где явно модифицируются системные контроллеры (MPU/кэш/clock-пайплайн) через HAL/CMSIS:
- MPU: регионы (в ветке LwIP) задают доступ/атрибуты для ETH DMA дескрипторов и heap LwIP.
- Кэш: включение I/D-cache после настройки MPU.
- RCC/PWR: выбор PLL, включение D2 SRAM3 clock и настройка делителей.

## Связи
- Использует: `main.c` (порядок вызовов), `hw_init.h` (API), HAL (`stm32h7xx_hal.h` и суб-модули), FreeRTOS только косвенно (через `Task_Display_Log`, но сама инициализация RTOS тут не делается).
- I2C/OLED: `ssd1306.h`, `ssd1306_fonts.h`, дескриптор `hi2c1`.
- UART лог: `UART_Log()` (вынесено в интерфейс `hw_init.h`), дескриптор `huart3`.
- USB: `usb_device.h` и `MX_USB_Device_Init()` при `USE_WEBUSB`.
- RNG/crypto: `RNG_HandleTypeDef` + `HAL_RNG_Init()` при `USE_CRYPTO_SIGN`.
- Сборочные флаги: `USE_LWIP`, `SKIP_OLED`, `USE_WEBUSB`, `USE_CRYPTO_SIGN`.
