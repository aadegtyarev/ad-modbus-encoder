# ad-modbus-encoder
Modbus энкодер на ESP8266

## Прошивка
Прошивка написана в Arduino, исходники в папке  `fw/src-smodbus-encoder`.

## Плата

### JLCPCB
Файлы для заказа плат на jlcpcb лежат в `pcb/jlcpcb files`.

Files for ordering boards in jlcpcb are located in the `pcb/jlcpcb files` folder.

### Исходники
Для редактирования исходник плат нужно:
1. [KiCad](https://www.kicad.org/) — среда проектирования.
2. Mini-360_DCDC_LM2596_module.kicad_mod — футпринт для Mini-360, лежит в `pcb/src/modules`.
3. [kicad-jlcpcb-tools](https://github.com/Bouni/kicad-jlcpcb-tools) — генерирует файлы для jlcpcb.
