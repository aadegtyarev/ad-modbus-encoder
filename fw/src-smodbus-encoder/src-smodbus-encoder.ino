/*
 Прошивка для платы NodeMCU на ESP8266, в других платах надо будет изменить
 конфигурацию пинов Дополнительно установить библиотеки: modbus-esp8266 и
 SimpleTimer
*/

#include <ModbusRTU.h>
#include <SimpleTimer.h>

// Адреса Modbus регистров
#define REG_BUTTON 0       // состояние кнопки
#define REG_BUTTON_COUNT 1 // счётчик нажатий на кнопку
#define REG_ENCODER 2      // состояние энкодера

#define SLAVE_ID 1 // Modbus адрес устройства

// конфигурация пинов
#define FLOW_PIN D3 // пин контроля направления приёма/передачи
#define PIN_A D0      // сигнал A энкодера
#define PIN_B D1      // сигнал B энкодера
#define PIN_BUTTON D2 // сигнал состояния кнопки

ModbusRTU mb;
SimpleTimer firstTimer(5); // запускаем таймер, который будем использовать для
                           // опроса кнопок и энкодера

boolean buttonWasUp = true; // флаг того, что кнопка отпущена
int encoderValue = 0;       // значение энкодера
int buttonCount = 0;        // счётчик нажатий на кнопку
int fadeAmount =
    10; // шаг, с которым будет изменяться значение энкодера в регистре
unsigned char encoder_A, encoder_B,
    encoder_A_prev = 0; // состояние сигналов энкодера

void setup() {
  Serial.begin(115200, SERIAL_8N2); // задаём парамеры связи
  mb.begin(&Serial);
  mb.begin(&Serial, FLOW_PIN); // включаем контроль направления приёма/передачи

  mb.slave(SLAVE_ID);
  mb.addHreg(REG_ENCODER);
  mb.addHreg(REG_BUTTON_COUNT);
  mb.addCoil(REG_BUTTON);

  mb.Hreg(REG_ENCODER, encoderValue);
  mb.Hreg(REG_BUTTON_COUNT, 0);
  mb.Coil(REG_BUTTON, 0);

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
    checkEncoder();
    checkButton();
    firstTimer.reset();
  }
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
      (encoder_A_prev)) { //   //Если уровень сигнала А низкий, и в предыдущем
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
