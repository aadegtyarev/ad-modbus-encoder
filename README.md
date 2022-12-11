# ad-modbus-encoder
Modbus энкодер на ESP8266

## Прошивка
Прошивка написана в Arduino, исходники в `fw/src-smodbus-encoder`, используются библиотеки *modbus-esp8266* и
 *SimpleTimer*.

## Плата
### Схема для сборки
Схемы в формате PDF в `pcb/pdf-schematic/`.

### JLCPCB
Файлы для заказа плат на jlcpcb в `pcb/jlcpcb files`.

### Исходники
Для редактирования исходников плат нужно:
1. [KiCad](https://www.kicad.org/) — среда проектирования.
2. Mini-360_DCDC_LM2596_module.kicad_mod — футпринт для Mini-360, лежит в `pcb/src/modules`.
3. [kicad-jlcpcb-tools](https://github.com/Bouni/kicad-jlcpcb-tools) — генерирует файлы для jlcpcb.
