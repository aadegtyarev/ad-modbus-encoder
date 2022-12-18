/* Подключаем библиотеки */
#include <ModbusRTU.h>    // modbus-esp8266
#include <EEPROM.h>       // работа с EEPROM
#include <SimpleTimer.h>  // порстой таймер

/* Объявления констант */
// Значение параметров связи по умолчанию
#define DEFAULT_MB_ADDRESS    1  // адрес нашего сервера
#define DEFAULT_MB_STOP_BITS  2  // количество стоповых битов
#define DEFAULT_MB_BAUDRATE   96 // скорость подключения/100

// Настройки энкодера по умолчанию
#define DEFAULT_ENC_STEP      5  // шаг изменения положения
#define DEFAULT_ENC_MIN       0   // минимальное значение
#define DEFAULT_ENC_MAX       100 // максимальное значение

// Номера Modbus регистров
#define REG_MB_ADDRESS    100 // адрес устройства на шине
#define REG_MB_STOP_BITS  101 // количество стоповых битов
#define REG_MB_BAUDRATE   102 // скорость подключения

// Номера регистров энкодера
#define REG_ENC_BUTTON        0 // состояние кнопки
#define REG_ENC_BUTTON_COUNT  1 // счётчик нажатий кнопки
#define REG_ENCODER           2 // регистр текущего положения
#define REG_ENC_STEP          3 // шаг изменения переменной положения энкодера
#define REG_ENC_MIN           4 // минимальное значение энкодера
#define REG_ENC_MAX           5 // максимальное значение энкодера

// Настройки EEPROM. Так как записывать в EEPROM будем числа типа uint16_t,
// которые на ESP8266/ESP32 занимают 4 байта, то каждое значение займёт две ячейки
#define EEPROM_SIZE           12 // мы займём 12 ячеек памяти
#define EEPROM_MB_ADDRESS     0 // адрес устройства
#define EEPROM_MB_STOP_BITS   2 // стоп-биты
#define EEPROM_MB_BAUDRATE    4 // скорость подключения

#define EEPROM_ENC_STEP       6 // шаг энкодера
#define EEPROM_ENC_MIN        8 // минимальное значение энкодера
#define EEPROM_ENC_MAX        10 // максимальное значение энкодера

// Описание входов-выходов
#define PIN_FLOW 4 // D2 пин контроля направления приёма/передачи,
// если в вашем преобразователе UART-RS485 такого нет —
// закоменнтируйте строку
#define BTN_SAFE_MODE 14 // D5 Кнопка сброса настроек подключения (SW)
#define PIN_A 12      // D6 сигнал A энкодера (CLK)
#define PIN_B 13      // D7 сигнал B энкодера (DT)
#define PIN_BUTTON BTN_SAFE_MODE // состояние кнопки

/* Переменные */
// Связь
uint16_t mbAddress = DEFAULT_MB_ADDRESS; // modbus адрес устройства
uint16_t mbStopBits = DEFAULT_MB_STOP_BITS; // количество стоповых битов
uint16_t mbBaudrate = DEFAULT_MB_BAUDRATE; // скорость подключения modbus

// Энкодер
volatile bool flagEncBtnPressed = false; // флаг события нажатия на кнопку
volatile bool flagEncChanged = false;    // флаг события изменения энкодера

uint16_t encBtnCount = 0;   // счётчик нажатий на кнопку
int16_t encValue = 0;       // значение энкодера
uint16_t encStep = DEFAULT_ENC_STEP; // шаг, с которым будет изменяться значение энкодера в регистре
uint16_t encMin = DEFAULT_ENC_MIN;   // минимальное значение
uint16_t encMax = DEFAULT_ENC_MAX;   // максимальное значение

/*Прочие настройки */
ModbusRTU mb;
SimpleTimer sysTimer(5); // запускаем таймер с интервалом 5 мс

// говорим компилятору, что эту функции надо поместить в RAM
void ICACHE_RAM_ATTR pin_button_pressed();
void ICACHE_RAM_ATTR pin_b_changed();

/* Функции инициализации  и главный цикл */
// Настройка устройства
void setup() {
  io_setup();               // настраиваем входы/выходы
  eeprom_setup();           // настраиваем EEPROM
  check_safe_mode();        // проверяем, не надо ли нам в безопасный режим с дефолтными настройками
  modbus_setup();           // настраиваем Modbus
  read_encoder_settings();  // читаем настройки энкодера
}

// Главный цикл
void loop() {
  mb.task();
  yield();
}

void yield() {
  if (sysTimer.isReady()) { // выполняется раз в 5мс
    check_enc_button(); // проверяем состояние кнопки
    check_encoder();    // проверяем изменения сигналов энкодера
    sysTimer.reset(); // сбрасываем таймер
  }
}

// Настройка входов-выходов
void io_setup() {
  pinMode(BTN_SAFE_MODE, INPUT_PULLUP);
  pinMode(PIN_B, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_BUTTON), pin_button_pressed, CHANGE); // нажатие на кнопку
  attachInterrupt(digitalPinToInterrupt(PIN_B), pin_b_changed, CHANGE); // вращение энкодера
}

// Иниализаця UART и конфигурирование Modbus
void modbus_setup() {
  Serial.begin(convert_baudrate(mbBaudrate), convert_stop_bits_to_config(mbStopBits)); // задаём парамеры связи
  mb.begin(&Serial);
  mb.begin(&Serial, PIN_FLOW); // включаем контроль направления приёма/передачи
  mb.slave(mbAddress); // указываем адрес нашего сервера

  /* Описываем регистры */
  // настройки связи
  mb.addHreg(REG_MB_ADDRESS);   // адрес устройства на шине
  mb.addHreg(REG_MB_STOP_BITS); // стоповые биты
  mb.addHreg(REG_MB_BAUDRATE);  // скорость подключения

  // энкодер
  mb.addHreg(REG_ENCODER);          // текущее значение
  mb.addIsts(REG_ENC_BUTTON);       // состояние кнопки
  mb.addIreg(REG_ENC_BUTTON_COUNT); // счётчик нажатий

  mb.addHreg(REG_ENC_STEP); // шаг изменения энкодера
  mb.addHreg(REG_ENC_MIN);  // минимально возможно значение
  mb.addHreg(REG_ENC_MAX);  // максимально возможное значение

  mb.addHreg(96);  //
  mb.addHreg(97);  //
  mb.addHreg(98);  //
  mb.addHreg(99);  //

  /*Инициализация регистров*/
  // записываем в регистры текущие значения адреса, стоповых битов и скорости
  mb.Hreg(REG_MB_ADDRESS, mbAddress);
  mb.Hreg(REG_MB_STOP_BITS, mbStopBits);
  mb.Hreg(REG_MB_BAUDRATE, mbBaudrate);

  // записываем значения по умолчанию в регистры стостояния энкодера
  mb.Hreg(REG_ENCODER, encValue); // значение энкодера
  mb.Ists(REG_ENC_BUTTON, 0); // состояние кнопки
  mb.Ireg(REG_ENC_BUTTON_COUNT, encBtnCount); // счётчик нажатий на кнопку

  // записываем текущие значения в регистры настроек энкодера
  mb.Hreg(REG_ENC_STEP, encStep);
  mb.Hreg(REG_ENC_MIN, encMin);
  mb.Hreg(REG_ENC_MAX, encMax);

  /* Назначение колбек функций на изменение регистров */
  // параметры связи
  mb.onSetHreg(REG_MB_ADDRESS, callback_set_mb_reg);
  mb.onSetHreg(REG_MB_STOP_BITS, callback_set_mb_reg);
  mb.onSetHreg(REG_MB_BAUDRATE, callback_set_mb_reg);

  // настройки энкодера
  mb.onSetHreg(REG_ENCODER, callback_set_enc_reg);
  mb.onSetHreg(REG_ENC_STEP, callback_set_enc_reg);
  mb.onSetHreg(REG_ENC_MIN, callback_set_enc_reg);
  mb.onSetHreg(REG_ENC_MAX, callback_set_enc_reg);
}

// Настройка параметров EEPROM
void eeprom_setup() {
  EEPROM.begin(EEPROM_SIZE);
}

// Чтение настроек Modbus: сперва читаем из EEPROM,
// а если там пусто, то берём значения по умолчанию
void read_modbus_settings() {
  EEPROM.get(EEPROM_MB_ADDRESS, mbAddress);
  if (mbAddress == 0xffff) {
    mbAddress = DEFAULT_MB_ADDRESS;
  }

  EEPROM.get(EEPROM_MB_STOP_BITS, mbStopBits);
  if (mbStopBits == 0xffff) {
    mbStopBits = DEFAULT_MB_STOP_BITS;
  }

  EEPROM.get(EEPROM_MB_BAUDRATE, mbBaudrate);
  if (mbBaudrate == 0xffff) {
    mbBaudrate = DEFAULT_MB_BAUDRATE;
  };
}

// Чтение настроек энкодера из EEPROM
void read_encoder_settings() {
  EEPROM.get(EEPROM_ENC_STEP, encStep);
  if (encStep == 0xffff) {
    encStep = DEFAULT_ENC_STEP;
  }

  EEPROM.get(EEPROM_ENC_MIN, encMin);
  if (encMin == 0xffff) {
    encMin = DEFAULT_ENC_MIN;
  }

  EEPROM.get(EEPROM_ENC_MAX, encMax);
  if (encMax == 0xffff) {
    encMax = DEFAULT_ENC_MAX;
  }
}

/* Обработчики прерываний */
// Нажатие на кнопку
void pin_button_pressed() {
  flagEncBtnPressed = true;
}

// Вращение энкодера
void pin_b_changed() {
  flagEncChanged = true;
}

/* Функции работы с периферией */
// Проверка состояние кнопки безопасного режима
void check_safe_mode() {
  if (digitalRead(BTN_SAFE_MODE)) { // Если нажата кнопка безопасного режима, то не считываем настройки связи из EEPROM
    read_modbus_settings();
  }
}

// Проверка флага нажатия кнопки и обработчик события
void check_enc_button() {
  if (flagEncBtnPressed) {
    bool btnState = !digitalRead(PIN_BUTTON); // считываем состояние кнопки

    mb.Ists(REG_ENC_BUTTON, btnState); // записываем состояние кнопки в регистр

    if (btnState) { // если кнопка нажата
      encBtnCount++; // увеличиваем счётчик нажатий
      mb.Ireg(REG_ENC_BUTTON_COUNT, encBtnCount); // записываем счётчик нажатий в регистр
    }
    flagEncBtnPressed = false; // сбрасываем флаг
  }
}

// проверка флага вращения вала энкодера и обработчик события
void check_encoder() {
  if (flagEncChanged) {
    // в зависимости от направления уменьшаем или увеличиваем значение переменной энкодера
    encValue += (digitalRead(PIN_B) == digitalRead(PIN_A) ? encStep : -encStep);

    if (encValue > encMax) {
      encValue = encMax;
    };
    if (encValue < encMin) {
      encValue = encMin;
    };

    mb.Hreg(REG_ENCODER, encValue); // записываем значение в регистр
    flagEncChanged = false; // сбрасываем флаг
  }
}

/* Вспомогательные функции */
// Колбек функция в которой мы записываем полученные по Modbus регистры в EEPROMM.
// Не забываем проверять записываемые значения на корректность, иначе мы можем потерять
// связь с устройством
uint16_t callback_set_mb_reg(TRegister* reg, uint16_t val) {
  switch (reg->address.address) {
    case REG_MB_ADDRESS: // если записываем регистр с адресом
      if (val > 0 && val < 247) { // проверяем, что записываемое число корректно
        write_eeprom(EEPROM_MB_ADDRESS, val); // записываем значение в EEPROM
      } else {
        val = reg->value;
      }
      break;
    case REG_MB_STOP_BITS: // если регистр со стоповыми битами
      if (val == 1 || val == 2) {
        write_eeprom(EEPROM_MB_STOP_BITS, val);
      } else {
        val = reg->value;
      }
      break;
    case REG_MB_BAUDRATE: // если регистр со скоростью
      uint16_t correctBaudRates[] = {12, 24, 48, 96, 192, 384, 576, 1152};
      if (contains(val, correctBaudRates, 8)) {
        write_eeprom(EEPROM_MB_BAUDRATE, val);
      } else {
        val = reg->value;
      }
      break;
  }
  return val;
}

// Колбек функция, где мы обрабатываем запись в значений в регистр
uint16_t callback_set_enc_reg(TRegister* reg, uint16_t val) {
  switch (reg->address.address) {
    case REG_ENCODER: // регистр со значением энкодера
      if (val >= encMin && val <= encMax) {
        encValue = val; // мы только меняем текущее его состояние в программе, но не пишем это значение в EEPROM
      } else {
        val = reg->value;
      }
      break;
    case REG_ENC_STEP: // регистр с шагом
      if (val > 0) {
        encStep = val;
        write_eeprom(EEPROM_ENC_STEP, encStep);
      } else {
        val = reg->value;
      }
    case REG_ENC_MIN: // регистр с минимумом
      encMin = val;
      write_eeprom(EEPROM_ENC_MIN, encMin);
      break;
    case REG_ENC_MAX: // регистр с минимумом
      encMax = val;
      write_eeprom(EEPROM_ENC_MIN, encMax);
      break;
  }
  return val;
}

// запись значения в EEPROM
void write_eeprom(uint8_t eepromm_address, uint16_t val) {
  EEPROM.put(eepromm_address, val);
  EEPROM.commit();
}

// Конвертер стоповых битов в настройки типа SerialConfig.
// Я не стал реализовывать все возможные варианты
// и давать настраивать чётность и количество битов данных, так как
// почти никто эти параметры при работе по протоколу Modbus не меняет
SerialConfig convert_stop_bits_to_config(uint16_t stopBits) {
  return (stopBits == 2) ? SERIAL_8N2 : SERIAL_8N1;
}

// Конвертер значения скорости. Для экономии места во флеше и регистрах, мы храним
// значение скорости, делённое на 100. То есть вместо 9600, мы храним 96.
// Эта функция умножает значение на 100.
uint32_t convert_baudrate(uint16_t baudrateValue) {
  return baudrateValue * 100;
}

// Функция, которая находит вхождение числа в массив
bool contains(uint16_t a, uint16_t arr[], uint8_t arr_size) {
  for (uint8_t i = 0; i < arr_size; i++) if (a == arr[i]) return true;
  return false;
}
