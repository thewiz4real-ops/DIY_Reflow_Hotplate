# Calibration and tuning

## Early method
The first version used a mechanical hotplate knob and a laser thermometer to find a repeatable 100 C preheat setting.

## Sensor problems found during the build
- a 6 mm gap between the hotplate and the aluminum plate created a large mismatch between sensed temperature and actual top surface temperature
- sensor placement inside the gap did not reflect the real working surface
- the sensor wire itself transferred heat and distorted readings

## Experiments
1. fiberglass gasket at the edge of the gap
2. moving the sensor toward the center
3. drilling a hole and fixing the sensor with ceramic cement
4. moving the sensor to the top surface
5. removing the gap entirely

The fiberglass gasket helped slightly, but not enough. The center-mounted ceramic-cement attempt also failed to solve the problem because the sensor wiring still had to pass through the gap.

The biggest clue was realizing that the sensor wire trapped between the plates was conducting heat into the sensor and creating a large reading error.

## Current result
The final fix was removing the gap completely, stacking the old aluminum plate with the newer one, screwing both directly to the hotplate, and placing the sensor on top. That made the reflow behavior much more believable, although the firmware tuning still needs more work.

## Still to do
- final sensor location tuning
- firmware tuning for overshoot / runaway control
- settings page adjustments for easier tuning
- real calibration table between setpoint and surface temperature

## Recommended next calibration step
- measure with a surface thermocouple placed directly on the work area
- compare real plate temperature against the displayed temperature
- record offset and overshoot at several setpoints before attempting unattended profiles
