#pragma once
// info
/*_________________________
  |NodeMCU PIN | GPIO PIN |
  |-----------------------|
  |    D0      |     16   |
  |    D1      |     5    |
  |    D2      |     4    |
  |    D3      |     0    | // SDA
  |    D4      |     2    | // SCL
  |    D5      |     14   | // SDA  <---------ESP8266--------
  |    D6      |     12   | // SCL  <---------ESP8266--------
  |    D7      |     13   |
  |    D8      |     15   |
  |    D9      |     3    |
  |    D10     |     1    |
  =========================

GPIOs 6,7,8,9,10 unusable*
*/

//if I2C used
#if (OLED == 1) || (BMP_EXIST == 1) || (BH1750_EXIST == 1) || (HTU21_EXIST == 1) || (CCS811_EXIST == 1 ) 
  #define WIRESDA 14 //esp8266 sda
  #define WIRESCL 12 //esp8266 scl
#endif

#if MOSFETSENSORS == 1
  #define POWERPIN 4 // mosfet for powering sensors
#endif

#if DHT_EXIST == 1
  #define DHT_SENSOR_PIN 5
#endif //DHT_EXIST

#if SHT_EXIST == 1
  #define SHT_SDA 0
  #define SHT_SCL 2
#endif //SHT_EXIST

#if DALLAS_EXIST == 1
  #define ONE_WIRE_BUS 13
#endif //DALLAS_EXIST

#if ANALOG_SENSOR == 1
  #define ANALOG_PIN A0
#endif //ANALOG_SENSOR

#if USELED == 1
  #define INDICATORLED 5 // led indicator
#endif //USELED

#if MUX_EXIST == 1
  const int mux_control_pins[] = {14, 12, 13}; // control pins for mux S1,S2,S3
  const int mux_data_line = A0;                //esp8266 input pin
#endif //MUX_EXIST

#if WEBCONFIG == 1
  #if MUX_EXIST == 1
    #define MUX_BUTTON_PIN 0 //multiplexed pin for button if
  #else //MUX_EXIST
    #define BUTTON_PIN 5
  #endif //MUX_EXIST
#endif //WEBCONFIG
