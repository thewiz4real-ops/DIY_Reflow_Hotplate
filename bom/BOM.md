# Bill of Materials

This is the main human-readable BOM for the project. It merges the cleaned part list with the original rough purchase links so the important URLs, part numbers, and notes are all in one place.

Use `BOM.csv` if you want to sort or filter the same list in a spreadsheet.

## Main BOM

| Category | Item | Qty | Link / Source | Part number | Notes |
|---|---|---:|---|---|---|
| Hotplate | Day single burner hotplate, 18 cm, 1500 W | 1 | https://www.megaflis.no/kjokken/kjokkenapparater/diverse-kjokkenapparater/day-kokeplate-o18cm-1500w? |  | Main heater base |
| Mechanical | Aluminum top plate | 1 | https://www.aliexpress.com/item/1005009533760651.html |  | Builder note says 8 mm or 10 mm works; this build used 10 mm |
| Mechanical | Aluminum base plate | 1 | Local / custom stock |  | 500 x 500 x 10 mm main base plate |
| Mechanical | Secondary aluminum plate | 1 | Reused part |  | Added during the final no-gap thermal fix |
| Mechanical | Armature sheet | 1 | Local / custom stock |  | 2 mm aluminum sheet for the hand rest / armature |
| Motion | 80 mm gantry plate | TODO | https://www.temu.com/goods.html?_bg_fs=1&goods_id=601099519049945 |  | V-slot gantry plate used in the motion assembly |
| Motion | V-slot wheel kit | TODO | https://www.temu.com/no-en/3d-printer-additions-for--v-shaped--plate-set-special-skate-wheels-compatible-with-2020-2040-v-slot-profiles-g-606143246110085.html |  | Openbuild-style wheel set for 2020 / 2040 profiles |
| Motion | T-slot extrusion | TODO | https://www.temu.com/no-en/t-slot-2020-extrusion-european-standard-anodized-linear-rail-for-3d-printer-parts-and-cnc-diy-black-g-606341536023071.html |  | Black anodized extrusion for frame and microscope support; exact lengths still need measuring |
| Motion | 2040 extrusion, 500 mm | 1 | https://www.aliexpress.com/item/1005004784760394.htm |  | Explicitly mentioned in the raw notes |
| Motion | Extrusion bracket set | 1 kit | https://www.temu.com/goods.html?_bg_fs=1&goods_id=601099554813640 |  | Joint plates, T-nuts, and M5 x 8 mm screws for the frame |
| Electronics | ESP32-2432S028 CYD display board | 1 | https://www.aliexpress.com/item/1005008314695130.html |  | Main controller |
| Electronics | Crydom D2425 SSR | 1 | Digikey | D2425 / CC1006-ND | SSR relay, SPST-NO, 25 A, 24-280 V |
| Electronics | NTC thermistor | 1 | Digikey | NRBG105F3950B1F / 283-NRBG105F3950B1F-ND | 100k NTC, beta 3950, 1 percent |
| Electronics | IRLZ44NPBF MOSFET | 1 | Generic supplier / existing stock | IRLZ44NPBF | Used in the driver section |
| Electronics | Perfboard | 1 | Generic supplier / existing stock |  | Used for the final controller wiring |
| Electronics | JST-style connectors | TODO | Generic supplier / existing stock |  | Exact pitch and connector family still need checking |
| Electronics | Resistor assortment | 1 pack | Generic supplier / AliExpress |  | Raw note says generic resistor package from AliExpress |
| Power | 5 V phone charger | 1 | Reused part |  | Original build reused this for ESP32 power; safer enclosed PSU is recommended for copies |
| Power | Power switch from extension strip | 1 | https://www.biltema.no/bygg/elektro/grenkontakter/grenkontakt-jordet/grenuttak-med-bryter-5-veis-hvitt-15-m-2000048332 |  | Original build reused only the switch |
| Optics | Digital microscope | 1 | https://www.temu.com/goods.html?_bg_fs=1&goods_id=601099593977408 |  | DM9-style microscope with LED lighting |
| Fasteners | M4 hex-head screws | TODO | Local hardware store / existing stock |  | Used for mounting parts to tapped aluminum; exact lengths still need checking |
| Fasteners | Wood screws, 3 x 15 mm | 1 pack | https://www.biltema.no/bygg/festeelementer/treskruer/treskrue-3-x-15-200-stk-2000058078 |  | Exact usage location still needs documenting |
| Wiring | Hookup wire | TODO | Generic supplier / existing stock |  | Thin stranded wire used like manual traces on perfboard |
| Wiring | Mains cable | 1 | Generic supplier / existing stock |  | Needs proper strain relief and enclosure documentation |
| Consumables | UV resin | 1 | Generic supplier / existing stock |  | Used to secure and insulate some mains-side joints |
| Consumables | Ceramic cement | 1 | Generic supplier / existing stock |  | Used during one thermistor mounting experiment |
| Consumables | Fiberglass gasket | 1 | Generic supplier / existing stock |  | 6 mm; used in one thermal gap experiment |
| Tools | M4 tap | 1 | Local hardware store / existing stock |  | Used for tapping threaded holes in the aluminum parts |
| Tools | Profile drilling guide | 1 | https://www.temu.com/no-en/drilling-guide-for-vertical--designed-for-20-30-and-40-aluminum-profiles-featuring-sliders-and-an-m6-countersunk-step-drill-g-601103103107036.html |  | Optional helper for aluminum profiles |
| Tools | Laser thermometer | 1 | Existing tool |  | Used to find a repeatable early preheat point |
| Tools | Hot air gun | 1 | Existing tool |  | Used during rework and bench testing |

## Notes

- Exact counts for extrusion lengths, wheels, JST connectors, and some fasteners still need to be verified from the finished machine.
- The original rough purchase note file is still kept in `source-notes/mybom_raw_links.txt` as an archive, but the important link data is now merged into this file.
- For public safety, it is better to recommend a properly enclosed certified 5 V PSU instead of copying the reused phone-charger approach from the original build.
