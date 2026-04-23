# Connector map

This is the current firmware-side I/O map gathered from the checked-in code.

## ESP32 controller pins

| Function | Pin / Part | Notes |
|---|---|---|
| NTC sensor ADC input | GPIO 35 | Input-only ADC pin |
| Heater output | GPIO 27 | Drives SSR or MOSFET stage, active high in current code |
| Touch IRQ | GPIO 36 | XPT2046 touch controller |
| Touch MOSI | GPIO 32 | XPT2046 touch controller |
| Touch MISO | GPIO 39 | XPT2046 touch controller |
| Touch CLK | GPIO 25 | XPT2046 touch controller |
| Touch CS | GPIO 33 | XPT2046 touch controller |

## Sensor network

| Item | Value | Notes |
|---|---|---|
| Thermistor | 100k NTC | Beta 3950 |
| Fixed resistor | 10k | Used in the divider math in firmware |

## Build notes

This file focuses on the firmware-side pin map.

The physical wire routing, connector placement, and mains layout are shown more clearly in the project video and the broader build notes than in a single fixed table.
