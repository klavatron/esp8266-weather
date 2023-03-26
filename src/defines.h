#pragma once
#include "Arduino.h"

/*
 * #define SOME_FEATURE 0 
 * bool some_feature_error = false;
 * 
 * #if SOME_FEATURE == 1
 *    #include ...
 *    float ...
 *    ....
 * #endif //SOME_FEATURE
 */

#define DEBUG 1          // serial logging enabled
#define USELED 0         // led indicator enabled
#define USE_SLEEP_MODE 1 // powersaving sleep mode enabled
#define MOSFETSENSORS 0  // using mosfet to powering sensors
#define WIFI 1           // wifi connection enabled
#define OLED 0           // i2c oled screen enabled
#define NARODMON 1       // sending data to narodmon enabled
#define BMP_EXIST 0      // i2c bmp075 presure sensor enabled
#define BH1750_EXIST 0   // i2c bh1750 light sensor enabled
#define HTU21_EXIST 0    // i2c HTU21 humidity sensor enabled (address 0x40)
#define CCS811_EXIST 0   // (dont use it) i2c CCS811 eCO2 sensor enabled.
#define DALLAS_EXIST 0   // onewire ds18b20 sensors enabled
#define ANALOG_SENSOR 0  // Analog sensor
#define DHT_EXIST 0      // dht11 sensor enabled
#define SHT_EXIST 0      // sht1x sensor enabled
#define MUX_EXIST 0      // analog multiplexer
#define WEBCONFIG 1      // enable web configurator. In case of esp-201 it may not work

#include "pins.h"

String floatToString(float src, char decimal_point = '.'); //Transform the float value to a string with custom decimal point symbol
void readSensors();                                        //read sensors, collect data to query string
unsigned long lastUpdateMillis;                            //providing time delay between sensors data update
unsigned long currentUpdateMillis;
const unsigned long updateInterval = 30000; // 30000 = 30 sec
bool update_flag = false;                   //if ready to update
void runOnce();                             // main function for deep sleep mode

//error flags if errors detected
bool oled_error = false;
bool bmp_error = false;
bool bh1750_error = false;
bool htu21_error = false;
bool ccs811_error = false;
bool dallas_error = false;
bool dht_error = false;
bool sht_error = false;
bool wifi_error = false;

#if DEBUG == 1
  const uint32_t SERIAL_SPEED=115200;
#endif //DEBUG

//if using pin to powering sensors
#if MOSFETSENSORS == 1
  void turnSensorsON();
  void turnSensorsOFF();
#endif //MOSFETSENSORS

#if USE_SLEEP_MODE == 1
  #define SLEEPING_TIME 600e6 // 20 sec 20e6; 600e6 - 10 min
  void checkResetInfo();
#endif //USE_SLEEP_MODE

//if using pin for led indicator
#if USELED == 1
  void ledBlink(int, int); // blink led (duration, count)
#endif //USELED

#if NARODMON == 1
  #include "narodmon.cfg.h"
  String POST_string = "";      //query string
  bool send_allow_flag = false; // allow http request
  unsigned long lastSendMillis;
  unsigned long currentSendMillis;
#endif //NARODMON

#if WIFI == 1
  #include <ESP8266WiFi.h>
  #include "wifi.cfg.h"
  #if WEBCONFIG == 1
    #include <WiFiClient.h>
    #include <ESP8266WebServer.h>
    #include <ESP8266mDNS.h>
    #include <ESP8266HTTPUpdateServer.h>

    bool isConfigMode = false;
    ESP8266WebServer httpServer(80);
    ESP8266HTTPUpdateServer httpUpdater;
  #endif //WEBCONFIG
#endif //WIFI

#if DALLAS_EXIST == 1
  #include <OneWire.h>
  #include <DallasTemperature.h>
  #define TEMPERATURE_PRECISION 12

  OneWire oneWire(ONE_WIRE_BUS);
  DallasTemperature dallas_sensors(&oneWire);

  int countOfDallasTerm = 0; //number of detected 18b20 thermometers
  DeviceAddress tempDeviceAddress;
  void printAddress(DeviceAddress);
  String aGetTempAddress(DeviceAddress);
  String aGetTemperature(DeviceAddress);
#endif //DALLAS_EXIST

#if BMP_EXIST == 1 // <-------- i2c
  #include <Wire.h>
  #include <Adafruit_Sensor.h>
  #include <Adafruit_BMP085_U.h>
  Adafruit_BMP085_Unified presureSensor = Adafruit_BMP085_Unified(10085);
  sensors_event_t event;
  void getBMPsensor();
#endif //BMP_EXIST

#if BH1750_EXIST == 1 // <-------- i2c
  #include <Wire.h>
  #include <BH1750.h>
  BH1750 lightMeter(0x23);
  float lux;
#endif //BH1750_EXIST

#if DHT_EXIST == 1
  #include <Adafruit_Sensor.h>
  #include <DHT.h>
  #define DHTTYPE DHT11
  //#define DHTTYPE    DHT22     // DHT 22 (AM2302)
  //#define DHTTYPE    DHT21     // DHT 21 (AM2301)
  DHT dht(DHT_SENSOR_PIN, DHTTYPE);
  float dht_t = 0.0;
  float dht_h = 0.0;
#endif //DHT_EXIST

#if SHT_EXIST == 1
  #include <SHT1x-ESP.h>
  SHT1x sht(SHT_SDA, SHT_SCL, SHT1x::Voltage::DC_3_3v);
  float sht_t_c = 0.0;
  float sht_t_f = 0.0;
  float sht_h = 0.0;
#endif //SHT_EXIST

#if HTU21_EXIST == 1 // <-------- i2c
  #include <Wire.h>
  #include "Adafruit_HTU21DF.h"
  Adafruit_HTU21DF htu21 = Adafruit_HTU21DF();

  float htu21_h = 0.0;
  float htu21_t = 0.0;
#endif //HTU21_EXIST

#if CCS811_EXIST == 1 // <-------- i2c
  #include <Wire.h>
  #include "DFRobot_CCS811.h"
  /*
 * IIC address default 0x5A, the address becomes 0x5B if the ADDR_SEL is soldered.
 */
//DFRobot_CCS811 CCS811(&Wire, /*IIC_ADDRESS=*/0x5A);
  DFRobot_CCS811 ccs;;
  float ccs811_eco2 = 0.0;
  float ccs811_tvoc = 0.0;
#endif //CCS811_EXIST

#if OLED == 1 // <-------- i2c
  #include <Wire.h>
  #include <Adafruit_GFX.h>
  #include <Adafruit_SSD1306.h>

  #define SCREEN_WIDTH 128 // OLED display width, in pixels
  #define SCREEN_HEIGHT 64 // OLED display height, in pixels

  Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
  void displayDraw();
#endif //OLED

#if MUX_EXIST == 1
  String muxPrefix = "A"; //prefix for mux sensors i.e. A0,A1...A7 for A
  void muxSwitchTo(int);
#endif // MUX_EXIST
