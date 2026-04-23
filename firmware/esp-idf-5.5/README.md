# ESP-IDF 5.5 hotplate project

This project keeps the UI and control logic in Arduino-style C++, but runs inside an ESP-IDF project using Arduino as a component.

## What is included
- `main/main.cpp` contains the hotplate app
- raw XPT2046 touch handling is built into `main.cpp`
- `TEMP_OFFSET_C` is stored in Preferences / NVS
- settings selection uses one large `NEXT` button
- touch reading uses median filtering for better tap stability

## One thing you still need
Clone `TFT_eSPI` into `components/TFT_eSPI`.

Example:
```bash
cd components
git clone https://github.com/Bodmer/TFT_eSPI.git
```

Then copy the working `User_Setup.h` / `User_Setup_Select.h` into that library, because the display wiring and driver config live there.

## Build steps
```bash
idf.py set-target esp32
idf.py build
idf.py -p PORT flash monitor
```

## Notes
- Arduino is pulled in by `main/idf_component.yml`
- the placeholder `components/` folder is included so the `TFT_eSPI` checkout has a clear home
- touch uses these pins:
  - IRQ 36
  - MOSI 32
  - MISO 39
  - CLK 25
  - CS 33
- if touch still lands off near the edges, recalibrate:
  - `XMIN`
  - `XMAX`
  - `YMIN`
  - `YMAX`
