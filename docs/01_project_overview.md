# Project overview

## Why this was built

This project started after building the Betax Hex, a large PCB with heavy copper layers. Hot-air-only reflow was hard because the board absorbed heat too fast. Solder cooled quickly, alignment was poor, and rework was uncomfortable.

The target was a setup that is:
- stable
- safe to handle
- controllable
- repeatable
- usable under a microscope
- suitable for both repair work and reflow

## Final goal

One machine that can be used as:
- a heat plate for repair and preheating
- a reflow plate when needed
- a safer microscope work area with a hand-rest

## Build path

The project did not start as the final rail-based machine.

The first version was a 1500 W cooking hotplate with a 2 mm aluminum top plate and a PCB holder. Because the hotplate only had a mechanical thermostat, a laser thermometer was used to find and mark a repeatable preheat position around 100 C.

That first version proved the thermal idea, but it was awkward and unsafe to use under a microscope. Burned fingers and poor ergonomics pushed the project toward a full mechanical redesign with a hand rest, motion system, rotating work area, and ESP32 control box.

## Full build log

The repo keeps the key files and lessons learned. For the full sequence of fabrication and assembly steps, use the YouTube video linked from the main `README.md`.
