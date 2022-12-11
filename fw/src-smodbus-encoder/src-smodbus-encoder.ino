/*
  Прошивка для платы ESP8266, в других платах надо будет изменить
  конфигурацию пинов Дополнительно установить библиотеки: modbus-esp8266 и
  SimpleTimer
*/

#include <ModbusRTU.h>
#include <SimpleTimer.h>
#include <EEPROM.h>

// системные настройки
#define EEPROM_SIZE 3
#define EEPROM_SLAVE_ID 0
#define EEPROM_BAUDRATE 1

// адреса Modbus регистров
#define REG_BUTTON 0       // состояние кнопки
#define REG_BUTTON_COUNT 1 // счётчик нажатий на кнопку
#define REG_ENCODER 2      // состояние энкодера
#define REG_SLAVE_ID 128 // modbus адрес
#define REG_BAUDRATE 110 // скорость подключения modbus

// конфигурация пинов
#define FLOW_PIN 4 // D2 пин контроля направления приёма/передачи
#define PIN_A 12      // D6 сигнал A энкодера
#define PIN_B 13      // D7 сигнал B энкодера
#define PIN_BUTTON 14 // D5 состояние кнопки

ModbusRTU mb;
SimpleTimer firstTimer(5); // запускаем таймер, который будем использовать для
// опроса кнопок и энкодера

uint8_t slaveId = 1; // modbus адрес устройства
uint16_t baudrateValue = 96; // скорость подключения modbus
boolean buttonWasUp = true; // флаг того, что кнопка отпущена
int16_t encoderValue = 0;       // значение энкодера
uint16_t buttonCount = 0;        // счётчик нажатий на кнопку
uint8_t fadeAmount =
  10; // шаг, с которым будет изменяться значение энкодера в регистре
int16_t encoderA, encoderB,
        encoderAPrev = 0; // состояние сигналов энкодера

void setup() {
  read_settings();
  modbus_setup();

  pinMode(PIN_A, INPUT);
  pinMode(PIN_B, INPUT);
  pinMode(PIN_BUTTON, INPUT_PULLUP);
}

void loop() {
  mb.task();
  yield();
}

void yield() {
  if (firstTimer.isReady()) {
    check_encoder();
    check_button();
    firstTimer.reset();
  }
}

void read_settings() {
  EEPROM.begin(EEPROM_SIZE);

  if (EEPROM.read(EEPROM_SLAVE_ID) == 0xff || EEPROM.read(EEPROM_BAUDRATE) == 0xff) {
    init_settings();
  }

  EEPROM.get(EEPROM_SLAVE_ID, slaveId);
  EEPROM.get(EEPROM_BAUDRATE, baudrateValue);

}

void write_settings(uint8_t eepromm_address, uint16_t val) {
  EEPROM.put(eepromm_address, val);
  EEPROM.commit();
}

void init_settings() {
  EEPROM.put(EEPROM_SLAVE_ID, slaveId);
  EEPROM.put(EEPROM_BAUDRATE, baudrateValue);
  EEPROM.commit();
}

void modbus_setup() {
  Serial.begin(baudrateValue * 100, SERIAL_8N2); // задаём парамеры связи baudrateValue * 100
  mb.begin(&Serial);
  mb.begin(&Serial, FLOW_PIN); // включаем контроль направления приёма/передачи

  mb.slave(slaveId);
  mb.addHreg(REG_ENCODER);
  mb.addHreg(REG_BUTTON_COUNT);
  mb.addHreg(REG_SLAVE_ID);
  mb.addHreg(REG_BAUDRATE);
  mb.addCoil(REG_BUTTON);

  mb.Hreg(REG_ENCODER, encoderValue);
  mb.Hreg(REG_BUTTON_COUNT, 0);
  mb.Hreg(REG_SLAVE_ID, slaveId);
  mb.Hreg(REG_BAUDRATE, baudrateValue);
  mb.Coil(REG_BUTTON, 0);

//  mb.addHreg(99); //debug
//  mb.Hreg(99, 0);

  mb.onSetHreg(REG_SLAVE_ID, callback_set);
  mb.onSetHreg(REG_BAUDRATE, callback_set);
}

uint16_t callback_set(TRegister* reg, uint16_t val) {

  switch (reg->address.address) {
    case REG_SLAVE_ID:
      if (val > 0 && val < 247) {
        write_settings(EEPROM_SLAVE_ID, val);
      }
      break;
    case REG_BAUDRATE:
      uint16_t correctBaudRates[] = {12, 24, 48, 96, 192, 384, 576, 1152};
      if (contains(val, correctBaudRates, 8)) {
        write_settings(EEPROM_BAUDRATE, val);
      }
      break;
  }
  return val;
}

bool contains(uint16_t a, uint16_t arr[], uint8_t arr_size) {
  for (uint8_t i = 0; i < arr_size; i++) if (a == arr[i]) return true;
  return false;
}

void check_button() {
  if (buttonWasUp && !digitalRead(PIN_BUTTON)) {
    buttonCount++; // увеличиваем счётчик нажатий

    mb.Coil(REG_BUTTON, 1); // записываем в регистр состояния кнопка 1
    mb.Hreg(REG_BUTTON_COUNT,
            buttonCount); // записываем в регистр значение счётчика нажатий
  } else {
    if (buttonWasUp) {
      mb.Coil(REG_BUTTON, 0); // записываем в регистр состояния кнопка 0
    }
  }

  buttonWasUp = digitalRead(PIN_BUTTON); // сохраняем флаг, что кнопка отпущена
}

void check_encoder() {
  encoderA = digitalRead(PIN_A); // проверяем сигнал A
  encoderB = digitalRead(PIN_B); // проверяем сигнал B

  if ((!encoderA) &&
      (encoderAPrev)) { //   //если уровень сигнала А низкий, и в предыдущем
    //   цикле он был высокий
    if (encoderB) {
      // вращение по часовой стрелке — увеличиваем значение в регистре
      if (encoderValue + fadeAmount <= 100)
        encoderValue += fadeAmount;
    } else {
      // вращение против часовой стрелки — уменьшаем значение в регистре
      if (encoderValue - fadeAmount >= 0)
        encoderValue -= fadeAmount;
    }
  }

  encoderAPrev =
    encoderA; // сохраняем состояние сигнала A для следующего цикла
  mb.Hreg(REG_ENCODER, encoderValue); // отправляем текущее значение энкодера
}
