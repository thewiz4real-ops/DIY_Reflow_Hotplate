# Firmware and UI

## Repo layout
- Arduino sketch: `firmware/arduino/hotplate_controller/hotplate_controller.ino`
- ESP-IDF project: `firmware/esp-idf-5.5/`

## Hardware stack
- ESP32 touchscreen board
- TFT_eSPI for display
- XPT2046 for touch
- Preferences / NVS for saved settings

## Why not LVGL
LVGL caused configuration and version mismatch problems during development, so the firmware moved to a simpler TFT_eSPI + XPT2046 setup.

The main problems were missing `lv_conf.h` setup and LVGL v8/v9 mismatch issues during compile-time testing.

## Display bring-up problems

Early display tests had split-screen output, noise, and black-screen behavior.

The working fix was:
- use the `ILI9341_2` driver configuration
- enable `TFT_INVERSION_ON`

That combination gave a stable display on the CYD hardware used in this build.

## UI structure
- Manual tab
- Reflow tab
- Settings tab

## Manual mode
- user selects temperature
- ramped setpoint behavior helps reduce overshoot
- can be used like a repair / preheat mode

## Reflow mode
- full profile
- stage display
- graph / progress indicator
- pause behavior if the plate falls behind the profile

## Settings
- approach band
- burst window
- max power limit
- values saved in flash

## Shared control I/O
- NTC ADC input on GPIO 35
- heater output on GPIO 27
- XPT2046 touch on GPIO 36, 32, 39, 25, and 33

## UI optimization
- small-region redraw instead of full-screen redraw
- touch debouncing / better click handling
- improved touch stability from debounce and target locking behavior
- improved stability and reduced flicker

## Important setup note
`TFT_eSPI` relies on a working `User_Setup.h` / `User_Setup_Select.h` for the specific CYD display wiring and driver configuration.
