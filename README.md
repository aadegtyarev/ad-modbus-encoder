# ad-modbus-encoder
Modbus Encoder on ESP8266

## Firmware
The firmware is written in the Arduino environment, the sources are in the `fw/src-smodbus-encoder` folder.

## PCB Files

### JLCPCB
Files for ordering boards in jlcpcb are located in the `pcb/jlcpcb files` folder.

### PCB source files
To edit sources you need:
1. [KiCad](https://www.kicad.org/) — development environment.
2. Mini-360_DCDC_LM2596_module.kicad_mod — footprint для Mini-360. Located in the `pcb/src/modules` folder.
3. [kicad-jlcpcb-tools](https://github.com/Bouni/kicad-jlcpcb-tools) — generating files for jlcpcb.
