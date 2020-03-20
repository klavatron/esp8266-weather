#pragma once

  static const uint8_t   D0 = 16;
  //static const uint8_t D1 = 5;
  //static const uint8_t D2 = 4;
  //static const uint8_t D3 = 0;  // SDA
  //static const uint8_t D4 = 2;  // SCL
  static const uint8_t   D5 = 14; // SDA  <---------ESP8266--------
  static const uint8_t   D6 = 12; // SCL  <---------ESP8266--------
  //static const uint8_t D7 = 13;
  //static const uint8_t D8 = 15;
  //static const uint8_t D9 = 3;
  //static const uint8_t D10= 1;

//if I2C used
#if (OLED == 1) || (BMP_EXIST == 1) || (BH1750_EXIST == 1) || (HTU21_EXIST == 1)
    #define WIRESDA 14 //esp8266 sda
    #define WIRESCL 12 //esp8266 scl
#endif

#if MOSFETSENSORS == 1
  #define POWERPIN 4  // mosfet for powering sensors
#endif

#if DHT_EXIST == 1
    #define DHT11_SENSOR_PIN A0
#endif //DHT_EXIST

#if DALLAS_EXIST == 1
    #define ONE_WIRE_BUS 13
#endif //DALLAS_EXIST

#if ANALOG_SENSOR == 1
    #define ANALOG_PIN  A0
#endif //ANALOG_SENSOR

#if USELED == 1
    #define INDICATORLED  5  // led indicator
#endif //USELED

#if MUX_EXIST == 1
    const int mux_control_pins[] = {14, 12, 13}; // control pins for mux S1,S2,S3
    const int mux_data_line = A0; //esp8266 input pin
#endif

#if WEBCONFIG == 1
    #if MUX_EXIST == 1
        #define MUX_BUTTON_PIN 0 //multiplexed pin for button if
    #endif
    //#define BUTTON_PIN 5 todo

#endif
