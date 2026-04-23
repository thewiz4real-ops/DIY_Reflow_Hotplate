# Known issues and lessons learned

## Mechanical
- Y-axis was too tight until the wheel setup was changed
- the control box was first mounted too low and hit the X-axis, so it had to be moved higher

## Electrical
- wrong 3.3 V source caused long debugging sessions
- breadboard success did not guarantee perfboard success
- clear connector labeling matters
- simple power mistakes can look like sensor or firmware bugs

## Thermal
- air gap idea looked good in theory but acted more like an insulator
- sensor wire placement mattered more than expected
- sensor temperature and working surface temperature can differ a lot

## Firmware / display
- LVGL was more trouble than it was worth for this build
- the CYD display needed the right driver and inversion settings to stop split-screen / black-screen problems
- small partial redraws were much better than redrawing the full screen

## Build philosophy lesson
Many of the best decisions in this project came from actually using the machine:
- burned fingers
- awkward PCB handling
- cable drag
- wheel friction
- unstable temperature readings

That is also the strongest story in the repo: the build evolved because of real use.
