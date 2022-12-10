/*
 ModbusRTU.h from https://github.com/emelianov/modbus-esp8266
 Used NodeMCU ESP8266
*/

#include <ModbusRTU.h>

#define REG_BUTTON 0
#define REG_BUTTON_COUNT 1
#define REG_ENCODER 2
#define SLAVE_ID 1
#define RXTX_PIN D3
#define PIN_A D0
#define PIN_B D1
#define PIN_BUTTON D2

ModbusRTU mb;

boolean buttonWasUp = true;
int encoderValue = 0;
int buttonCount = 0;
int fadeAmount = 10;        // the step of changing the value in the register
unsigned long currentTime;
unsigned long loopTime;
unsigned char encoder_A;
unsigned char encoder_B;
unsigned char encoder_A_prev=0;

void setup() {
  Serial.begin(115200, SERIAL_8N2);
  mb.begin(&Serial);
  mb.begin(&Serial, RXTX_PIN);  //use RX/TX direction control pin
  
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

  currentTime = millis();
  loopTime = currentTime;   
}

void loop() {
  mb.task();
  yield();
}

void yield() {
  currentTime = millis();
  if(currentTime >= (loopTime + 5)){ // check every 5ms (200 Hz)

    // ENCODER
    encoder_A = digitalRead(PIN_A);     // check pin A
    encoder_B = digitalRead(PIN_B);     // check pin B
    if((!encoder_A) && (encoder_A_prev)){    // if the state has changed from positive to zero
      if(encoder_B) {
        // clockwise rotation — increment the value in the register
        if(encoderValue + fadeAmount <= 100) encoderValue += fadeAmount;               
      }   
      else {
        // counterclockwise rotation — decrement the value in the register
        // 
        if(encoderValue - fadeAmount >= 0) encoderValue -= fadeAmount;               
      }   

    }   
    encoder_A_prev = encoder_A;     //  save the value of pin A for the next cycle

    mb.Hreg(REG_ENCODER, encoderValue);    

    // BUTTON
    if (buttonWasUp && !digitalRead(PIN_BUTTON)) {
      delay(10);
      if (!digitalRead(PIN_BUTTON)){
        buttonCount++;
        
        mb.Coil(REG_BUTTON, 1);    
        mb.Hreg(REG_BUTTON_COUNT, buttonCount);                  
        }
      } else {
        if (buttonWasUp)
          mb.Coil(REG_BUTTON, 0); 
      }
        
    buttonWasUp = digitalRead(PIN_BUTTON);    
    loopTime = currentTime;  
  } 
}
