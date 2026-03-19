\page ssd1306_conf "ssd1306_conf: SSD1306 driver build-time configuration"

# `Drivers/ssd1306/ssd1306_conf.h`

<brief>Nagłówek `ssd1306_conf` definiuje konfigurację czasu kompilacji dla sterownika SSD1306: wiąże I2C1 (`hi2c1`) i adres 0x3C, ustawia geometrię wyświetlacza 128×32, włącza wymagany czcionkę (Font 6×8). Te makra są używane przez źródła sterownika i kod UI wywołujący.</brief>

## Przegląd

<brief>Nagłówek `ssd1306_conf` definiuje konfigurację czasu kompilacji dla sterownika SSD1306: wiąże I2C1 (`hi2c1`) i adres 0x3C, ustawia geometrię wyświetlacza 128×32, włącza wymagany czcionkę (Font 6×8). Te makra są używane przez źródła sterownika i kod UI wywołujący.</brief>

## Abstrakcja (synteza logiki)

Sterownik SSD1306 na bare-metal jest zazwyczaj kompilowany "dla konkretnego układu wyświetlacza" i "dla konkretnej magistrali/adresu". `ssd1306_conf.h` to punkt ustalenia takich parametrów, aby `task_display` i `task_display_minimal` mogły pracować z pojedynczym interfejsem `ssd1306_*` bez zajmowania się ustawieniami kompilacji.

## Przepływ logiki

To nie moduł runtime, ale zbiór makr:

| Makro | Wartość | Rola |
|---|---|---|
| `SSD1306_USE_I2C` | zdefiniowane | wybiera backend I2C |
| `SSD1306_I2C_PORT` | `hi2c1` | powiązanie z instancją HAL I2C |
| `SSD1306_I2C_ADDR` | `(0x3C<<1)` | 7-bitowy adres z factor shift I2C |
| `SSD1306_WIDTH`/`HEIGHT` | `128/32` | wymiary dla buforu/renderowania |
| `SSD1306_INCLUDE_FONT_6x8` | zdefiniowane | obecność wymaganej czcionki |

## Przerwania/rejestry

Brak ISR lub rejestrów: tylko makra konfiguracyjne do włączenia poprawnego kodu sterownika.

## Zależności

Bezpośrednie:
- `hi2c1` z `hw_init.c` (eksportowane w `hw_init.h`)

Stosowane do:
- `task_display.c`, `task_display_minimal.c` (poprzez dołączenie `ssd1306.h` i czcionek)

## Relacje

- `hw_init.md` (inicjalizacja I2C + `hi2c1`)
- `task_display.md` (renderowanie nad SSD1306)
