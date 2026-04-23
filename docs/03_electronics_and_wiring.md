# Electronics and wiring

## Main controller
- ESP32-2432S028 (CYD touchscreen board)

## Heater power switching
- Crydom D2425 solid-state relay (SSR)
- bolted to the aluminum chassis for heat transfer and grounding

## Temperature sensing
- 100k NTC sensor
- beta 3950
- original part note from the build: NRBG105F3950B1F

## Pin map

The current firmware-side I/O map is documented in `hardware/wiring/connector_map.md`.

## Prototyping path
1. Breadboard proof-of-concept
2. Bench testing with a desk lamp and hot air to validate the sensor, relay, and UI
3. Perfboard implementation
4. JST-style plug connections for cleaner wiring
5. Final enclosure assembly

## Perfboard wiring method
Instead of making long solder blobs between holes, thin stranded wire was used like a manual trace. The wire was soldered down along the full path to make the connection clean and solid.

## Main debugging issue
A signal/header pin was accidentally used where the board should have been powered from the correct 3.3 V rail. That caused the sensor supply to sag whenever the MOSFET / relay load switched, which showed up as false temperature spikes.

Moving the supply to the correct 3.3 V pin on the board fixed that instability.

## Power and enclosure
- the original build reused an old 5 V phone charger for ESP32 power
- mains wiring for the charger input and power switch was soldered and reinforced with UV resin
- the enclosure was 3D printed for the SSR, charger, main switch, and touchscreen opening
- hotplate body and aluminum chassis were grounded
- cable routing channels and zip ties were used to keep the mains wiring under control

## Mains wiring note
The original build reused a phone charger for 5 V and used mains wiring inside the box. Keep this part clearly marked as hazardous in the public repo.

Recommended public-facing note:
> If you want to replicate this build, use a properly enclosed certified 5 V PSU unless you are qualified to modify mains-powered supplies.
