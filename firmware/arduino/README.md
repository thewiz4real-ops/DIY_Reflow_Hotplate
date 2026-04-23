# Arduino Firmware

The Arduino version of the controller lives in `hotplate_controller/hotplate_controller.ino`.

## Expected libraries

- `TFT_eSPI`
- `XPT2046_Touchscreen`
- `Preferences`

## Notes

- this sketch targets the ESP32-2432S028 style CYD board
- touch and heater pins match the ESP-IDF version
- `TFT_eSPI` still needs the correct board-specific `User_Setup.h`
