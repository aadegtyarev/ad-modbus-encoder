/*
 Прошивка для платы NodeMCU на ESP8266, в других платах надо будет изменить
 конфигурацию пинов Дополнительно установить библиотеки: modbus-esp8266 и
 SimpleTimer
*/

#include <ModbusRTU.h>
#include <SimpleTimer.h>
#include <EEPROM.h>

// системные настройки
#define EEPROM_SIZE 2
#define EEPROM_SLAVE_ID 0

// адреса Modbus регистров
#define REG_BUTTON 0       // состояние кнопки
#define REG_BUTTON_COUNT 1 // счётчик нажатий на кнопку
#define REG_ENCODER 2      // состояние энкодера
#define REG_SLAVE_ID 128
//#define REG_BAUDRATE 110

// конфигурация пинов
#define FLOW_PIN D3 // пин контроля направления приёма/передачи
#define PIN_A D0      // сигнал A энкодера
#define PIN_B D1      // сигнал B энкодера
#define PIN_BUTTON D2 // сигнал состояния кнопки

ModbusRTU mb;
SimpleTimer firstTimer(5); // запускаем таймер, который будем использовать для
                           // опроса кнопок и энкодера

int slaveId = 0; // modbus адрес устройства
boolean buttonWasUp = true; // флаг того, что кнопка отпущена
int encoderValue = 0;       // значение энкодера
int buttonCount = 0;        // счётчик нажатий на кнопку
int fadeAmount =
    10; // шаг, с которым будет изменяться значение энкодера в регистре
unsigned char encoder_A, encoder_B,
    encoder_A_prev = 0; // состояние сигналов энкодера

void(* resetFunc) (void) = 0; // объявляем функцию reset 

void setup() {
  EEPROM.begin(EEPROM_SIZE);
  
  slaveId = EEPROM.read(EEPROM_SLAVE_ID);
  if (slaveId == 0xff){
    slaveId = 1;
  }

  modbus_setup();
  
  pinMode(PIN_A, INPUT);
  pinMode(PIN_B, INPUT);
  pinMode(PIN_BUTTON, INPUT_PULLUP);
}

void modbus_setup(){
  Serial.begin(115200, SERIAL_8N2); // задаём парамеры связи
  mb.begin(&Serial);
  mb.begin(&Serial, FLOW_PIN); // включаем контроль направления приёма/передачи

  mb.slave(slaveId);
  mb.addHreg(REG_ENCODER);
  mb.addHreg(REG_BUTTON_COUNT);
  mb.addHreg(REG_SLAVE_ID);
  mb.addCoil(REG_BUTTON);

  mb.Hreg(REG_ENCODER, encoderValue);
  mb.Hreg(REG_BUTTON_COUNT, 0);
  mb.Hreg(REG_SLAVE_ID, slaveId);
  mb.Coil(REG_BUTTON, 0);

  mb.onSetHreg(REG_SLAVE_ID, callbackSet);  
}

void loop() {
  mb.task();
  yield();
}

void yield() {
  if (firstTimer.isReady()) {
    checkEncoder();
    checkButton();
    firstTimer.reset();
  }
}

uint16_t callbackSet(TRegister* reg, uint16_t val) {
  slaveId = val;
  EEPROM.write(EEPROM_SLAVE_ID, slaveId);
  EEPROM.commit();
  return val;
}

void checkButton() {
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

void checkEncoder() {
  encoder_A = digitalRead(PIN_A); // проверяем сигнал A
  encoder_B = digitalRead(PIN_B); // проверяем сигнал B

  if ((!encoder_A) &&
      (encoder_A_prev)) { //   //если уровень сигнала А низкий, и в предыдущем
                          //   цикле он был высокий
    if (encoder_B) {
      // вращение по часовой стрелке — увеличиваем значение в регистре
      if (encoderValue + fadeAmount <= 100)
        encoderValue += fadeAmount;
    } else {
      // вращение против часовой стрелки — уменьшаем значение в регистре
      if (encoderValue - fadeAmount >= 0)
        encoderValue -= fadeAmount;
    }
  }

  encoder_A_prev =
      encoder_A; // сохраняем состояние сигнала A для следующего цикла
  mb.Hreg(REG_ENCODER, encoderValue); // отправляем текущее значение энкодера
}
