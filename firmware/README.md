# Firmware

Two firmware versions are included in this repo:

- `arduino/hotplate_controller/` for the Arduino IDE style sketch
- `esp-idf-5.5/` for the ESP-IDF 5.5 project using Arduino as a component

Both versions target the same ESP32 touchscreen controller and share the same basic feature set:

- Manual mode
- Reflow mode
- Settings page
- XPT2046 touch input
- NTC temperature reading
- SSR heater control
- settings saved in flash

## Shared pin summary

- NTC sensor: GPIO 35
- SSR output: GPIO 27
- touch IRQ: GPIO 36
- touch MOSI: GPIO 32
- touch MISO: GPIO 39
- touch CLK: GPIO 25
- touch CS: GPIO 33

## Important note

The display still depends on a correct `TFT_eSPI` configuration for the exact CYD board and panel driver.
