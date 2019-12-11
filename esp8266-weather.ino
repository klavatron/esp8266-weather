/*
*  This sketch sends data via HTTP POST requests to narodmon.ru service.
*  Copyright <klavatron> 2015-2019
*
*  ***DRAFT***
*           ESP8266 BoardVersion 2.6.2 https://github.com/esp8266/Arduino
*  Sensors:
*           bmp180  i2c   Adafruit BMP085 Unified 1.0.0 https://github.com/adafruit/Adafruit_BMP085_Unified
*           BH1750  i2c   Christopher Laws, March, 2013 https://github.com/claws/BH1750.git
*           HTU21   i2c   HTU21D Humidity Sensor Library Nathan Seidle SparkFun Electronics September 22nd, 2013
*           dth11         Adafruit DHT sensor library 1.3.0 https://github.com/adafruit/DHT-sensor-library
*           ds18b20 onewire DallasTemperature 3.7.6 https://github.com/milesburton/Arduino-Temperature-Control-Library.git
*           MQ4 (trigger)
*
*  You need to change:
            Remove .example for narodmon.cfg.h.example and wifi.cfg.h.example
*                      const char* ssid = "SSID" - name of wifi AP
*                      const char* password = "PASSWORD" - password of wifi AP
*                      String DEV_ID = "ABCDEFABCDEF"- hw address of device
*                      const char* host = "185.245.187.136" - ip of narodmon.ru
*
* If you don't use oled screen or debug logging, switch USE_SLEEP_MODE to 1 for powersaving.
*/


  static const uint8_t INDICATORLED   = 5;  // led indicator
  static const uint8_t POWERPIN   = 4;  // mosfet for powering sensors
  
  static const uint8_t D0   = 16;
  //static const uint8_t D1   = 5;
  //static const uint8_t D2   = 4;
  //static const uint8_t D3   = 0;  // SDA
  //static const uint8_t D4   = 2;  // SCL
  static const uint8_t D5   = 14; // SDA  <---------ESP8266--------
  static const uint8_t D6   = 12; // SCL  <---------ESP8266--------
  //static const uint8_t D7   = 13;
  //static const uint8_t D8   = 15;
  //static const uint8_t D9   = 3;
  //static const uint8_t D10  = 1;


#define DEBUG 1   // serial logging enabled
#if DEBUG == 1
    #define SERIAL_SPEED 115200
#endif

#define WIFI 1 // wifi connection enabled
#define MOSFETSENSORS 1 // using mosfet to powering sensors
#define OLED 0    // i2c oled screen enabled
#define NARODMON 1  // sending data to narodmon enabled
#define BMP_EXIST 1  // i2c bmp075 presure sensor enabled
#define BH1750_EXIST 1 // i2c bh1750 light sensor enabled
#define HTU21_EXIST 1  // i2c HTU21 humidity sensor enabled

//if I2C used
#if (OLED == 1) || (BMP_EXIST == 1) || (BH1750_EXIST == 1) || (HTU21_EXIST == 1)
    #define WIRESDA 14 //esp8266 sda
    #define WIRESCL 12 //esp8266 scl
#endif
//DHT11 is pure quality sensor, don't use it
#define DHT_EXIST 0   // dht11 sensor enabled
#if DHT_EXIST == 1
    #define DHT11_SENSOR_PIN A0;
#endif //DHT_EXIST

#define DALLAS_EXIST 1  // onewire ds18b20 sensors enabled
#if DALLAS_EXIST == 1
    #define ONE_WIRE_BUS 13
#endif //DALLAS_EXIST

#define MQ4_EXIST 0 // MQ4 methan sensor enabled
#if MQ4_EXIST == 1
    int METHAN_SENSOR_PIN = 12;
#endif //MQ4_EXIST

#define USE_SLEEP_MODE 1 // powersaving sleep mode enabled
#if USE_SLEEP_MODE == 1
    #define SLEEPING_TIME 600e6 // 20 sec 20e6; 600e6 - 10 min
    void check_reset_info();
#endif //USE_SLEEP_MODE

#define USELED 1  // led indicator enabled
#if USELED == 1
    void led_blink(int, int); // blink led (duration, count)
#endif //USELED

String float_to_sting(float); //convert float value to a proper string
void read_sensors(); //read sensors, collect data to query string
unsigned long last_update_millis;
unsigned long current_update_millis;
const unsigned long update_interval = 30000; // 30000 = 30 sec
bool update_flag = false;
void run_once(); // main function

#if NARODMON == 1
  #include "narodmon.cfg.h"
    String POST_string = ""; //query string
    bool send_allow_flag = false; // allow http request
    unsigned long last_send_millis;
    unsigned long current_send_millis;
#endif //NARODMON

#if WIFI == 1
    #include <ESP8266WiFi.h>
    #include "wifi.cfg.h"
    extern "C" {
        #include <user_interface.h>
    }
#endif //WIFI

#if DALLAS_EXIST == 1
    #include <OneWire.h>
    #include <DallasTemperature.h>
    #define TEMPERATURE_PRECISION 12

    OneWire oneWire(ONE_WIRE_BUS);
    DallasTemperature dallas_sensors(&oneWire);

    int countOfDallasTerm;
    DeviceAddress tempDeviceAddress;

    #if DEBUG == 1
    void printAddress(DeviceAddress da)
    {
        for (uint8_t i = 0; i < 8; i++)
        {
            if (da[i] < 16) Serial.print("0");
                Serial.print(da[i], HEX);
        }
    }
    #endif //DEBUG

    String aGetTemperature(DeviceAddress da)
    {
        float tempC = dallas_sensors.getTempC(da);
        return float_to_sting(tempC);
    }

    String aGetTempAddress(DeviceAddress da)
    {
        String temps="&";
        for (uint8_t i = 0; i < 8; i++)
        {
            int a,b =0;
            a = (int)floor(da[i]/16);
            b = (int)floor(da[i]%16);
            a < 10 ? temps+=(String)a : temps+=(String)(char((a-10)+'A'));
            b < 10 ? temps+=(String)b : temps+=(String)(char((b-10)+'A'));
        }
        temps += "=";
        return temps;
    }
#endif //DALLAS_EXIST

#if BMP_EXIST == 1 // <-------- i2c
    #include <Wire.h>
    #include <Adafruit_BMP085_U.h>
    Adafruit_BMP085_Unified presureSensor = Adafruit_BMP085_Unified(10085);

    void getBMPsensor(void)
    {
        sensor_t bmp085_sensor;
        presureSensor.getSensor(&bmp085_sensor);
    }
#endif //BMP_EXIST

#if BH1750_EXIST == 1 // <-------- i2c
    #include <Wire.h>
    #include <BH1750.h>
    BH1750 lightMeter;
#endif //BH1750_EXIST

#if DHT_EXIST == 1
    #include <dht.h>
    DHT dht11 = DHT();
#endif //DHT_EXIST

#if HTU21_EXIST == 1 // <-------- i2c
    #include <Wire.h>
    #include <HTU21D.h>
    HTU21D myHTU21D;
    float humidity =0.0;
    float temperature =0.0;
#endif //HTU21_EXIST

#if OLED == 1 // <-------- i2c
    #include <Wire.h>
    #include <Adafruit_GFX.h>
    #include <Adafruit_SSD1306.h>
    #define OLED_RESET LED_BUILTIN //4  <---------------------------------------
    Adafruit_SSD1306 display(OLED_RESET);

    #if (SSD1306_LCDHEIGHT != 64)
        #error("Height incorrect, please fix Adafruit_SSD1306.h!");
    #endif
    void display_draw();
#endif //OLED

void setup()
{
    #if DEBUG == 1
        Serial.begin(115200);
        Serial.print("\r\n\nSketch: "); Serial.println(__FILE__);
        Serial.println("Compiled: " __DATE__ ", " __TIME__ );
        Serial.println("\n=================== CONFIG ==================");
    #endif //DEBUG

    #if MOSFETSENSORS == 1
    //if using mosfet
      pinMode(POWERPIN, OUTPUT);
      digitalWrite(POWERPIN, HIGH);
      Serial.println("MOSFET init");
    #endif //MOSFETSENSORS

    delay(1000);

    last_update_millis = millis();

    #if NARODMON == 1
        last_send_millis = millis();
    #endif //NARODMON

    //if I2C used
    #if (OLED == 1) || (BMP_EXIST == 1) || (BH1750_EXIST == 1) || (HTU21_EXIST == 1)
        Wire.begin(WIRESDA, WIRESCL);
        delay(100);
    #endif

    #if USELED == 1
        pinMode(INDICATORLED, OUTPUT);
        digitalWrite(INDICATORLED, LOW);
    #endif //USELED

    #if OLED == 1
        display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3D (for the 128x64) //must be 0x3C <-----------------------------------------
        delay(100);
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(WHITE);
        display.setCursor(0,0);
        display.println("ESP weather");
        display.display();
        delay(500);
    #endif //OLED


    #if WIFI == 1
        #if OLED == 1
            display.clearDisplay();
            display.setCursor(0,0);
            display.println("Connecting to: ");
            display.setCursor(0,16);
            display.println(ssid);
            display.display();
        #endif //OLED

        #if DEBUG == 1
            #if NARODMON == 1
                Serial.print("Narodmon IP:        "); Serial.println(host);
            #endif //NARODMON
            Serial.print("Connecting to:      "); Serial.print(ssid);  Serial.print(" ");
        #endif //DEBUG

        WiFi.mode( WIFI_STA );
        WiFi.begin(ssid, password);

        #if OLED ==1
            display.setCursor(0,32);
        #endif //OLED
  
        while(WiFi.status() != WL_CONNECTED)
        {
            delay(1000);

            #if DEBUG == 1
            Serial.print(".");
            #endif //DEBUG

            #if OLED ==1
            display.print(".");
            display.display();
            #endif //OLED

            #if USELED == 1
            led_blink(50, 1);
            #endif //USELED
        }

        #if USELED == 1
            led_blink(100, 2);
        #endif //USELED

        #if DEBUG == 1
            Serial.println(" OK");
            Serial.print("IP address:         ");Serial.println(WiFi.localIP());
        #endif //DEBUG

        #if OLED == 1
            delay(50);
            display.println("OK");
            display.display();
            display.setCursor(0,48);
            display.print("IP:"); display.println(WiFi.localIP());
            display.display();
        #endif //OLED

        delay(500);
    #endif //WIFI

    #if DALLAS_EXIST == 1
        delay(50);
        dallas_sensors.begin();
        countOfDallasTerm = dallas_sensors.getDeviceCount();

        #if DEBUG == 1
            Serial.print("Locating dallas devices...\t");
            (countOfDallasTerm>0)?Serial.print(" OK\n"):Serial.print(" FAIL\n");
            Serial.print("Found "); Serial.print(countOfDallasTerm, DEC);Serial.println(" devices.");
        #endif //DEBUG

        for(int i=0; i<countOfDallasTerm; i++)
        {
            // Search the wire for address
            if(dallas_sensors.getAddress(tempDeviceAddress, i))
            {
                #if DEBUG == 1
                    Serial.print("Found device with address: ");
                    printAddress(tempDeviceAddress);
                    Serial.println();
                #endif //DEBUG
                // set the resolution to 9 bit (Each Dallas/Maxim device is capable of several different resolutions)
                dallas_sensors.setResolution(tempDeviceAddress, TEMPERATURE_PRECISION);
            }
            else
            {
                #if DEBUG == 1
                    Serial.print("Found ghost device at "); Serial.print(i, DEC);
                    Serial.print(" but could not detect address. Check power and cabling");
                #endif //DEBUG
            }
        }

    #endif //DALLAS_EXIST

    #if BMP_EXIST == 1
        delay(50);
        if(!presureSensor.begin())
        {
            #if DEBUG == 1
                Serial.println("No BMP180 detected.");
            #endif //DEBUG
        }
        else
        {
            #if DEBUG == 1
                Serial.println("BMP                 init");
            #endif //DEBUG
        }
        getBMPsensor();
    #endif //BMP_EXIST

    #if BH1750_EXIST == 1
        delay(50);
        lightMeter.begin(BH1750_ONE_TIME_HIGH_RES_MODE_2);
        #if DEBUG == 1
            Serial.println("BH1750              init");
        #endif //DEBUG
    #endif //BH1750_EXIST

    #if MQ4_EXIST == 1
        delay(50);
        pinMode(METHAN_SENSOR_PIN, INPUT);
        #if DEBUG == 1
            Serial.println("MQ4                 init");
        #endif //DEBUG
    #endif //MQ4_EXIST

    #if DHT_EXIST == 1
        delay(50);
        dht11.attach(DHT11_SENSOR_PIN);
        #if DEBUG == 1
            Serial.println("DHT11 init");
        #endif //DEBUG
    #endif //DHT_EXIST

    #if HTU21_EXIST == 1
        delay(50);
        #if defined(ARDUINO_ARCH_ESP8266) || (ESP8266_NODEMCU)
            // while (myHTU21D.begin(WIRESDA,WIRESCL) != true)
            #else
                while (myHTU21D.begin() != true)
        #endif

        #if DEBUG == 1
            Serial.println("Si7021              init");
        #endif //DEBUG
    #endif //HTU21_EXIST

    Serial.println("=============== END OF CONFIG ==============="); Serial.println("");
    delay(100);

    run_once();
} // setup

void loop()
{
#if USE_SLEEP_MODE != 1
    current_update_millis = millis();
    if(current_update_millis-last_update_millis>=update_interval)
    {
        last_update_millis = current_update_millis;
        update_flag = true;
    }

    // if it's time to update - read sensors, send data
    if(update_flag == true)
    {
        /*
        typedef enum {
        WL_NO_SHIELD        = 255,   // for compatibility with WiFi Shield library
        WL_IDLE_STATUS      = 0,
        WL_NO_SSID_AVAIL    = 1,
        WL_SCAN_COMPLETED   = 2,
        WL_CONNECTED        = 3,
        WL_CONNECT_FAILED   = 4,
        WL_CONNECTION_LOST  = 5,
        WL_DISCONNECTED     = 6
        } wl_status_t;
        */
        #if DEBUG == 1
        Serial.println("\r\n\n-->start");
        Serial.print("WIFI status code: "); Serial.println(WiFi.status());
        #endif //DEBUG


        read_sensors();

        #if USELED == 1
        led_blink(200, 3);
        #endif //USELED

        #if OLED ==1
        display_draw();
        #endif //OLED

        #if NARODMON == 1
        current_send_millis = millis();

        if(current_send_millis-last_send_millis>=send_period)
        {
            last_send_millis = current_send_millis;
            send_allow_flag = true;
        }
        else
        {
            #if DEBUG == 1
            Serial.print("Next delivery in ");Serial.print(round((send_period - (current_send_millis-last_send_millis))/1000));Serial.println(" seconds");
            #endif //DEBUG
        }

        // if it's time to send post message - send message
        if(send_allow_flag == true)
        {
            send_message(POST_string);
        }
        #endif //narodmon

        update_flag = false;
        #if DEBUG == 1
        Serial.println("-->end");
        #endif //DEBUG
    } //if(update_flag == true)
#endif //!USE_SLEEP_MODE
}

String float_to_sting(float src)
{
    String data = "";
    if(src<0) //check for minus sign
    {
        data= "-";
    }
    int a,b,c,tmp,tmp2=0;
    tmp=src*100;
    tmp2=abs(tmp);
    a=(int)round(tmp);
    b=(int)floor(a/100);
    c=(int)floor(a%100);
    data+= String(b);
    data+= ".";

    if(c >= 10)
    {
        data+= String(c);
    }
    else
    {
        data += "0";
        data+= String(c);
    }
    return data;
}

#if USE_SLEEP_MODE == 1
void goto_sleep()
{
    #if DEBUG == 1
        Serial.println("Do nothing for 10 seconds");  // This makes it easier to re-flash the chip later.
    #endif //DEBUG
    delay(10000);

    #if DEBUG == 1
        Serial.print("Off To Sleep for "); Serial.print(round(SLEEPING_TIME/60e6)); Serial.println(" min");
    #endif //DEBUG

    #if MOSFETSENSORS == 1
      digitalWrite(POWERPIN, LOW);
    #endif //MOSFETSENSORS

    led_blink(400, 3);
    ESP.deepSleep(SLEEPING_TIME, WAKE_RF_DEFAULT); //20 sec

    #if DEBUG == 1
        Serial.println("Why im not sleeping?");
    #endif //DEBUG
}
#endif //USE_SLEEP_MODE

#if USELED == 1
void led_blink(int duration, int count)
{
    for(int i=0; i<count; i++)
    {
        digitalWrite(INDICATORLED, HIGH);   // turn the LED on
        delay(duration);
        digitalWrite(INDICATORLED, LOW);   // turn the LED off
        delay(duration);
    }
}
#endif //USELED

#if NARODMON == 1
void send_message(String data)
{
    #if DEBUG == 1
        Serial.println("=============== send msg start ==============");
    #endif //DEBUG

    #if WIFI == 1
    if(WiFi.status() == WL_CONNECTED)
    {
        WiFiClient client;
        if (!client.connect(host, httpPort))
        {
            #if DEBUG == 1
                Serial.println("-------------- host unreachable -------------");
                Serial.println("               reboot in 10 sec              ");
                Serial.println("---------------------------------------------");
            #endif //DEBUG
            led_blink(50, 5);
            ESP.deepSleep(10000, WAKE_RF_DEFAULT); //sleep 10 sec
        }

        #if DEBUG == 1
            Serial.println("WIFI:               Connected");
        #endif //DEBUG

        client.print(String("POST http://narodmon.ru/post.php HTTP/1.0\r\nHost: narodmon.ru\r\nContent-Type: application/x-www-form-urlencoded\r\n"));
        client.print(String("Content-Length: " + String(data.length()) + "\r\n\r\n" + data + "\r\n\r\n"));

        #if DEBUG == 1
            Serial.println(String("POST http://narodmon.ru/post.php HTTP/1.0\r\nHost: narodmon.ru\r\nContent-Type: application/x-www-form-urlencoded\r\n"));
            Serial.println(String("Content-Length: " + String(data.length()) + "\r\n\r\n" + data + "\r\n\r\n"));
            Serial.println("Server reply begin =====>");
        #endif //DEBUG

        delay(50);
        while(client.available())
        {
            String line = client.readStringUntil('\r');

            #if DEBUG == 1
                Serial.print(line);
                Serial.print(String(line.length()));
            #endif //DEBUG

            #if OLED ==1
                display.clearDisplay();
                display.setCursor(0,0);
                display.println(line.substring(0,20));
                display.display();
            #endif //OLED

            delay(100);
        }
        #if DEBUG == 1
            Serial.println("Server reply end <===== ");
            Serial.println("closing connection");
        #endif //DEBUG

        client.stop();
        #if DEBUG == 1
            Serial.println("wait 3 sec...");
        #endif //DEBUG
        delay(3000);
    }
    else
    {
        #if DEBUG == 1
            Serial.println("------------- wifi not connected ------------");
            Serial.println("               reboot in 10 sec              ");
            Serial.println("---------------------------------------------");
        #endif // DEBUG
        led_blink(100, 5);
        ESP.deepSleep(10000, WAKE_RF_DEFAULT); //sleep 10 sec

        #if DEBUG == 1
            Serial.println("Why im not sleeping?");
        #endif //DEBUG
    }
    #endif //WIFI
    send_allow_flag = false;

    #if DEBUG == 1
        Serial.println("=============== send msg end ==============");
    #endif //DEBUG
}
#endif //NARODMON
// ###########################

void run_once()
{
    #if DEBUG == 1
       Serial.println("run_once:           Going to check sensors");
    #endif //DEBUG

    read_sensors();

    #if DEBUG == 1
       Serial.println("run_once:           Sensors checked");
    #endif //DEBUG

    #if OLED ==1
        display_draw();
    #endif //oled

    #if USE_SLEEP_MODE == 1
        check_reset_info(); // send request to server if waked after sleep
    #endif // USE_SLEEP_MODE

    #if USELED == 1
        led_blink(100, 3);
    #endif // USELED

    #if NARODMON == 1
        POST_string = "";
    #endif //NARODMON

    #if USE_SLEEP_MODE == 1
        goto_sleep();
    #endif //USE_SLEEP_MODE
} //run_once

void read_sensors()
{
   #if DEBUG == 1
       Serial.println("read_sensors:       Going to collect data");
    #endif //DEBUG
    #if NARODMON == 1
        POST_string = "ID=";
        POST_string += DEV_ID;
    #endif //NARODMON



    #if DALLAS_EXIST == 1
       #if DEBUG == 1
          Serial.println("read_sensors:       Dallas");
       #endif //DEBUG
       
        dallas_sensors.requestTemperatures();

        #if NARODMON == 1
            for(int i=0;i<countOfDallasTerm; i++)
            {
                if(dallas_sensors.getAddress(tempDeviceAddress, i))
                {
                  #if DEBUG == 1
                    Serial.print(aGetTempAddress(tempDeviceAddress)); Serial.print(":  ");  Serial.println(aGetTemperature(tempDeviceAddress));
                  #endif //DEBUG
                    POST_string +=aGetTempAddress(tempDeviceAddress);
                    POST_string +=aGetTemperature(tempDeviceAddress);
                }
            }
        #endif //NARODMON
    #endif //DALLAS_EXIST



    #if BMP_EXIST == 1
       #if DEBUG == 1
          Serial.println("read_sensors:       BMP180");
       #endif //DEBUG
       
        sensors_event_t event;
        presureSensor.getEvent(&event);
        if (event.pressure)
        {
            float presure = 0.0;
            presure = event.pressure;

            #if DEBUG == 1
                Serial.print("bmp180 pressure     ");   Serial.print(presure);   Serial.println(" hPa");
            #endif //DEBUG

            #if NARODMON == 1
                POST_string += "&"; POST_string += DEV_ID;
                POST_string += "01="; POST_string += float_to_sting(presure);
            #endif //NARODMON

            float temperature = 0.0;
            presureSensor.getTemperature(&temperature);

            #if DEBUG == 1
                Serial.print("bmp180 temperature  ");  Serial.print(temperature);  Serial.println(" C");
            #endif // DEBUG

            #if NARODMON == 1
                POST_string += "&"; POST_string += DEV_ID;
                POST_string += "03="; POST_string += float_to_sting(temperature);
            #endif //NARODMON
        }
        else
        {
            Serial.println("Sensor error");
        }
    #endif //BMP_EXIST

    #if BH1750_EXIST == 1

           #if DEBUG == 1
               Serial.println("read_sensors:       BH1750");
           #endif //DEBUG

        lightMeter.begin(BH1750_ONE_TIME_HIGH_RES_MODE_2);
        uint16_t lux = lightMeter.readLightLevel(); //54000
        #if NARODMON == 1
            POST_string += "&"; POST_string += DEV_ID;
            POST_string += "05="; POST_string += (String)lux;
        #endif //NARODMON
        #if DEBUG == 1
            Serial.print("BH1750              ");
            Serial.print(lux);
            Serial.println(" lux");
        #endif //DEBUG
    #endif //BH1750_EXIST



    #if DHT_EXIST == 1
        #if DEBUG == 1
           Serial.println("read_sensors: DHT");
        #endif //DEBUG
        
        dht11.update();
        float dht11_t=0.0;
        float dht11_h=0.0;

        switch (dht11.getLastError())
        {
            case DHT_ERROR_OK:
                char msg[128];
                dht11_t=dht11.getTemperatureInt();
                dht11_h=dht11.getHumidityInt();
                sprintf(msg, "Temperature = %dC, Humidity = %d%%", dht11_t, dht11_h);

                #if DEBUG == 1
                    Serial.println(msg);
                #endif //DEBUG
                break;

            case DHT_ERROR_START_FAILED_1:
                #if DEBUG == 1
                    Serial.println("Error: start failed (stage 1)");
                #endif //DEBUG
                break;

            case DHT_ERROR_START_FAILED_2:
                #if DEBUG == 1
                    Serial.println("Error: start failed (stage 2)");
                #endif //DEBUG
                break;

            case DHT_ERROR_READ_TIMEOUT:
                #if DEBUG == 1
                    Serial.println("Error: read timeout");
                #endif //DEBUG
                break;

            case DHT_ERROR_CHECKSUM_FAILURE:
                #if DEBUG == 1
                    Serial.println("Error: checksum error");
                #endif //DEBUG
                break;
        }
        #if NARODMON == 1
            POST_string += "&";  POST_string += DEV_ID;
            POST_string += "02=";  POST_string += float_to_sting(dht11_h);
            POST_string += "&"; POST_string += DEV_ID;
            POST_string += "04="; POST_string += float_to_sting(dht11_t);
        #endif //NARODMON
    #endif //DHT_EXIST

    #if MQ4_EXIST == 1
       #if DEBUG == 1
          Serial.println("read_sensors: MQ4");
       #endif //DEBUG
       
        int d_sensor_val = digitalRead(METHAN_SENSOR_PIN);

        #if NARODMON == 1
            POST_string += "&"; POST_string += DEV_ID;
            POST_string += "06="; POST_string += float_to_sting(DHT.humidity);
        #endif //NARODMON
    #endif //MQ4_EXIST

    #if HTU21_EXIST == 1
       #if DEBUG == 1
          Serial.println("read_sensors:       HTU21");
       #endif //DEBUG
       
        float htu21_h = 0.0;
        float htu21_t = 0.0;
        htu21_h = myHTU21D.readHumidity();
        htu21_t = myHTU21D.readTemperature();

        #if DEBUG == 1
            Serial.print("HTU humidity        ");
            Serial.print(htu21_h);
            Serial.println(" +- 2% RH");
            Serial.print("HTU temp            ");
            Serial.print(htu21_t);
            Serial.println(" +- 0.5 deg.C");
        #endif //DEBUG

        #if NARODMON == 1
            POST_string += "&"; POST_string += DEV_ID;
            POST_string += "07="; POST_string += float_to_sting(htu21_h);
            POST_string += "&"; POST_string += DEV_ID;
            POST_string += "08="; POST_string += float_to_sting(htu21_t);
        #endif //NARODMON
    #endif //HTU21_EXIST

    #if NARODMON == 1
        #if DEBUG == 1
            Serial.println();
            Serial.print("Compiled string: ");   Serial.println(POST_string);
        #endif //DEBUG
    #endif  //NARODMON
    update_flag = false;
} //read_sensors

#if OLED == 1
    void display_draw()
    {
        display.clearDisplay();

        #if HTU21_EXIST == 1
            display.setCursor(0,32);
            display.print("H:"); display.print(htu21_h);
            display.setCursor(64, 32);
            display.print("T:"); display.print(htu21_t);
        #endif // HTU21_EXIST

        #if BH1750_EXIST == 1
            display.setCursor(0, 48);
            display.print("L:");display.print((String)lux);
        #endif //BH1750_EXIST

        #if BMP_EXIST == 1
            display.setCursor(64, 48);
            display.print("P:"); display.print(event.pressure);
        #endif //BMP_EXIST
    }
#endif //OLED

#if USE_SLEEP_MODE == 1
    void check_reset_info()
    {
        /*enum rst_reason {
        *  REASON_DEFAULT_RST    = 0,  // normal startup by power on
        *  REASON_WDT_RST      = 1,  // hardware watch dog reset
        *  REASON_EXCEPTION_RST  = 2,  // exception reset, GPIO status won’t change
        *  REASON_SOFT_WDT_RST     = 3,  // software watch dog reset, GPIO status won’t change
        *  REASON_SOFT_RESTART   = 4,  // software restart ,system_restart , GPIO status won’t change
        *  REASON_DEEP_SLEEP_AWAKE = 5,  // wake up from deep-sleep
        *  REASON_EXT_SYS_RST      = 6   // external system reset
        *};
        */
        #if WIFI == 1
            rst_info *resetInfo;
            resetInfo = ESP.getResetInfoPtr();

            #if DEBUG == 1
                Serial.print("Wake code:          ");Serial.print(resetInfo->reason);Serial.print("- ");
            #endif //DEBUG

            //switch reset reason
            switch (resetInfo->reason)
            {
                case 0: // первичная загрузка
                    #if DEBUG == 1
                        Serial.println("Normal startup");
                    #endif //DEBUG
                    #if USELED == 1
                        led_blink(500, 2);
                    #endif //USELED
                    break;

                case 1:
                    #if DEBUG == 1
                        Serial.println("Hardware watch dog reset");
                    #endif //DEBUG
                    #if USELED == 1
                        led_blink(500, 4);
                    #endif //USELED
                    break;

                case 2:
                    #if DEBUG == 1
                        Serial.println("Exception reset, GPIO status won’t change");
                    #endif //DEBUG
                    #if USELED == 1
                        led_blink(500, 5);
                    #endif //USELED
                    break;

                case 3:
                    #if DEBUG == 1
                        Serial.println("Software watch dog reset, GPIO status won’t change");
                    #endif //DEBUG
                    #if USELED == 1
                        led_blink(500, 5);
                    #endif //USELED
                    break;

                case 4:
                    #if DEBUG == 1
                        Serial.println("Software restart ,system_restart , GPIO status won’t change");
                    #endif //DEBUG
                    #if USELED == 1
                        led_blink(500, 5);
                    #endif //USELED
                    break;

                case 5:    //wake up
                    #if DEBUG == 1
                        Serial.println("Wake up from deep-sleep");
                    #endif //DEBUG
                    #if USELED == 1
                        led_blink(500, 3);
                    #endif //USELED
                    #if DEBUG == 1
                        Serial.println("\nPreparing request");
                    #endif //DEBUG
                    #if NARODMON == 1
                        send_message(POST_string);
                    #endif //NARODMON
                    break;

                case 6:    //button reset
                    #if DEBUG == 1
                        Serial.println("External system reset");
                    #endif //DEBUG
                    #if USELED == 1
                        led_blink(500, 1);
                    #endif //USELED
                    break;
            }
        #endif //WIFI
    }
#endif //USE_SLEEP_MODE
