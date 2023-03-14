/*
*  This sketch sends data via HTTP POST requests to narodmon.ru service.
*  Copyright <klavatron> 2015-2023
*
*  ***DRAFT***
*           ESP8266 BoardVersion 2.6.3 https://github.com/esp8266/Arduino
*
*  List of supported sensors:
*                         Adafruit Unified Sensor Driver 1.1.4 https://github.com/adafruit/Adafruit_Sensor
*           bmp180  i2c   Adafruit BMP085 Unified 1.1.0 https://github.com/adafruit/Adafruit_BMP085_Unified
*                         Adafruit GFX Library 1.1.8 https://github.com/adafruit/Adafruit-GFX-Library
*           OLED    i2c   2.0.1 Adafruit SSD1306 oled driver library for monochrome 128x64 and 128x32 
*                               displays https://github.com/adafruit/Adafruit_SSD1306
*           BH1750  i2c   1.1.4 Christopher Laws https://github.com/claws/BH1750.git
*           !HTU21  i2c   1.0.2 Adafruit HTU21DF Library with modified begin() 
*             -->               function https://github.com/klavatron/Adafruit_HTU21DF_Library.git
*           CCS811  i2c   1.1.1 Adafruit CCS811 https://github.com/adafruit/Adafruit_CCS811
*           dth11         Adafruit DHT sensor library 1.3.8 https://github.com/adafruit/DHT-sensor-library
*           SHT1x   2wire       beegee-tokyo  https://github.com/beegee-tokyo/SHT1x-ESP
*           ds18b20 onewire DallasTemperature 3.7.6 https://github.com/milesburton/Arduino-Temperature-Control-Library.git
*           *Analog        You need to use voltage divider to keep analog voltage in range 0..1v (LoLin NodeMCU already has it)
*           *Multiplexer   8-ch analog Multiplexer hc4051. You need to use voltage divider to keep analog 
*                               voltage in range 0..1v (LoLin NodeMCU already has it). May be usefull for polling 7-8 Capacitive
*                               Soil Moisture sensors from one ESP device
*                           
*           Remove .example extention for wifi.cfg.h.example file
*           edit:
*             ssid = "YOUR-WIFI" - name of existing wifi AP
*             password = "YOUR-PASSWORD" - password of wifi AP
*
*           If you want to send data to narodmon.ru remove .example extention for narodmon.cfg.h.example file
*           edit:
*             narodmonDevId = "ABCDEF012345"- hw address of device
*             host = "185.245.187.136" - ip of narodmon.ru
*
*           Edit defines.h to chose your sensors. Edit pins.h to assign chosen sensord to pins of ESP8266.
*           You can't use all sensors in one device because there is not enough available pins for them.
*
* If you don't use oled screen or debug logging, switch USE_SLEEP_MODE to 1 for powersaving.
*/
#include "Arduino.h"
#include "defines.h"

void setup()
{
  #if DEBUG == 1
    Serial.begin(SERIAL_SPEED);
    Serial.print("\r\n\nSketch: ");
    Serial.println(__FILE__);
    Serial.println("Compiled: " __DATE__ ", " __TIME__);
    Serial.println("\n=================== CONFIG ==================");
  #endif //DEBUG

  #if MOSFETSENSORS == 1 //if using mosfet for powering sensors
    pinMode(POWERPIN, OUTPUT);
    turnSensorsON();
  #endif //MOSFETSENSORS

  delay(100);

  #if MUX_EXIST == 1
    #if DEBUG == 1
      Serial.println("Multiplexer         init");
    #endif //DEBUG

    pinMode(mux_data_line, INPUT);

    for (int i = 0; i < 3; i++)
    {
      pinMode(mux_control_pins[i], OUTPUT);
    }
  #endif // MUX_EXIST

  #if WEBCONFIG == 1
    // check if webconfig button pressed
    #if MUX_EXIST == 1
      muxSwitchTo(MUX_BUTTON_PIN);
      if (analogRead(A0) > 300)
      {
        isConfigMode = true;
      }
    #else // MUX_EXIST
      pinMode(BUTTON_PIN, INPUT_PULLUP);
      if (!digitalRead(BUTTON_PIN))
      {
        isConfigMode = true;
      }
    #endif // MUX_EXIST
    // ENDOF check if webconfig button pressed

    if (isConfigMode) //if mux MUX_BUTTON_PIN is high -> configboot
    {
      #if DEBUG == 1
        Serial.println("Entering configuration mode");                                                                        
      #endif // DEBUG

      WiFi.mode(WIFI_STA);
      WiFi.begin(ssid, password);

      // Wait for connection
      while ((WiFi.status() != WL_CONNECTED) && sta_connection_attempts)
      {
        delay(1000);

        #if DEBUG == 1
          Serial.print(".");
        #endif // DEBUG

        sta_connection_attempts--;
      }

      //if cant connect in STA mode, create AP
      if (sta_connection_attempts <= 0)
      {
        #if DEBUG == 1
          Serial.println("AP mode");
        #endif //DEBUG

        WiFi.mode(WIFI_AP);
        WiFi.softAP(ap_ssid, ap_password);
        IPAddress myIP = WiFi.softAPIP();

        #if DEBUG == 1
          Serial.print("AP IP address: ");
          Serial.println(myIP);
        #endif // DEBUG
      }
      else
      {
        #if DEBUG == 1
          Serial.println("");
          Serial.print("Connected to ");
          Serial.println(ssid);
          Serial.print("IP address: ");
          Serial.println(WiFi.localIP());
        #endif // DEBUG
      }

      if (!MDNS.begin(mdnshost))
      {
        #if DEBUG == 1
          Serial.println("Error setting up MDNS responder!");
        #endif // DEBUG

        while (1)
        {
          delay(1000);
        }
      }

      #if DEBUG == 1
        Serial.println("mDNS responder started");
        Serial.println("In configuration mode");
      #endif // DEBUG

      httpUpdater.setup(&httpServer, update_path, update_username, update_password);
      httpServer.begin();

      MDNS.addService("http", "tcp", 80);

      #if DEBUG == 1
        Serial.print("WEB UPDATER on ");
        Serial.println(WiFi.localIP());
        Serial.printf("HTTPUpdateServer ready! Open http://%s.local%s in your browser and login with username '%s' and password '%s'\n", mdnshost, update_path, update_username, update_password);
      #endif // DEBUG

      while (1) // loop()
      {
        httpServer.handleClient();
        MDNS.update();
      }
    }
    delay(100);
  #endif // WEBCONFIG

  #if USE_SLEEP_MODE == 0 // if not using deep sleep
    lastUpdateMillis = millis();
  #endif // USE_SLEEP_MODE

  #if NARODMON == 1
    lastSendMillis = millis();
  #endif //NARODMON

  //if I2C used
  #if (OLED == 1) || (BMP_EXIST == 1) || (BH1750_EXIST == 1) || (HTU21_EXIST == 1) || (CCS811_EXIST == 1)
    Wire.begin(WIRESDA, WIRESCL);
    delay(100);
  #endif

  #if USELED == 1
    pinMode(INDICATORLED, OUTPUT);
    digitalWrite(INDICATORLED, LOW);
  #endif //USELED

  #if OLED == 1
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
    { // bug. always TRUE // Address 0x3D for 128x64, 0x3C for 128x32 and some 128x64 oled displays
      #if DEBUG == 1
        Serial.println("OLED initialization error");
      #endif
      oled_error = true;
    }
    else
    {
      #if DEBUG == 1
        Serial.println("OLED initialization Ok");
      #endif
    }

    if (!oled_error)
    {
      delay(300);
      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(WHITE);
      display.setCursor(0, 0);
      display.println("ESP weather");
      display.display();
      delay(500);
    }
  #endif //OLED

  #if WIFI == 1
    #if OLED == 1
      if (!oled_error)
      {
        display.clearDisplay();
        display.setCursor(0, 0);
        display.println("Connecting to: ");
        display.setCursor(0, 16);
        display.println(ssid);
        display.display();
      }
    #endif //OLED

    #if DEBUG == 1
      #if NARODMON == 1
        Serial.print("Narodmon IP:        ");
        Serial.println(host);
      #endif //NARODMON

      Serial.print("Connecting to:      ");
      Serial.print(ssid);
      Serial.print(" ");
    #endif //DEBUG

    //WiFi.forceSleepWake();
    WiFi.mode(WIFI_STA);
    delay(100);

    #if OLED == 1
      if (!oled_error)
      {
        display.setCursor(0, 32);
      }
    #endif //OLED

    if (WiFi.SSID())
    {
      #if DEBUG == 1
        Serial.print("x");
      #endif //DEBUG
      WiFi.begin();
    }
    else
    {
      #if DEBUG == 1
        Serial.print("X");
      #endif //DEBUG
      WiFi.begin(ssid, password);
    }
    delay(100);
    int attempts = 0;

    while (WiFi.status() != WL_CONNECTED)
    {
      attempts++;
      if (attempts == 10)
      {
        WiFi.begin(ssid, password);
      }
      if (attempts > 20)
      {
        wifi_error = true;
        break;
      }
      delay(1000);

      #if DEBUG == 1
        Serial.print(".");
      #endif //DEBUG

      #if OLED == 1
        if (!oled_error)
        {
          display.print(".");
          display.display();
        }
      #endif //OLED

      #if USELED == 1
        ledBlink(50, 1);
      #endif //USELED
    }

    #if USELED == 1
      ledBlink(100, 2);
    #endif //USELED

    #if DEBUG == 1
      if (!wifi_error)
      {
        Serial.println(" OK");
        Serial.print("IP address:         ");
        Serial.println(WiFi.localIP());
      }
      else
      {
        Serial.println("WiFi error");
      }
    #endif //DEBUG

    #if OLED == 1
      if (!oled_error)
      {
        if (!wifi_error)
        {
          delay(50);
          display.println("OK");
          display.display();
          display.setCursor(0, 48);
          display.print("IP:");
          display.println(WiFi.localIP());
          display.display();
        }
        else
        {
          delay(50);
          display.println("WiFi Err");
          display.display();
        }
      }
    #endif //OLED

    delay(500);
  #endif //WIFI

  #if DALLAS_EXIST == 1
    delay(50);
    dallas_sensors.begin();
    countOfDallasTerm = dallas_sensors.getDeviceCount();

    if (countOfDallasTerm <= 0)
      dallas_error = true;

    #if DEBUG == 1
      Serial.print("Locating dallas devices...\t");
      (countOfDallasTerm > 0) ? Serial.print(" OK\n") : Serial.print(" FAIL\n");
      Serial.print("Found ");
      Serial.print(countOfDallasTerm, DEC);
      Serial.println(" devices.");
    #endif //DEBUG

    if (!dallas_error)
    {
      for (int i = 0; i < countOfDallasTerm; i++)
      {
        // Search the wire for address
        if (dallas_sensors.getAddress(tempDeviceAddress, i))
        {
          #if DEBUG == 1
            Serial.print("Dallas thermometer: ");
            printAddress(tempDeviceAddress);
            Serial.println();
          #endif //DEBUG
          // set the resolution to 9 bit (Each Dallas/Maxim device is capable of several different resolutions)
          dallas_sensors.setResolution(tempDeviceAddress, TEMPERATURE_PRECISION);
        }
        else
        {
          #if DEBUG == 1
            Serial.print("Found ghost device at ");
            Serial.print(i, DEC);
            Serial.print(" but could not detect address. Check power and cabling");
          #endif //DEBUG
        }
      }
    }
  #endif //DALLAS_EXIST

  #if BMP_EXIST == 1
    delay(50);
    if (!presureSensor.begin())
    {
      #if DEBUG == 1
        Serial.println("BMP                 fail");
      #endif //DEBUG
      bmp_error = true;
    }
    else
    {
      #if DEBUG == 1
        Serial.println("BMP                 init");
      #endif //DEBUG
      getBMPsensor();
    }
  #endif //BMP_EXIST

  #if BH1750_EXIST == 1
    delay(50);

    if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE_2))
    {
      #if DEBUG == 1
        Serial.println("BH1750              init");
      #endif //DEBUG
    }
    else
    {
      bh1750_error = true;
      #if DEBUG == 1
        Serial.println("BH1750              fail");
      #endif //DEBUG
    }
  #endif //BH1750_EXIST

  #if ANALOG_SENSOR == 1
    delay(50);
    pinMode(ANALOG_PIN, INPUT);

    #if DEBUG == 1
      Serial.println("Analog pin          init");
    #endif //DEBUG

  #endif //ANALOG_SENSOR

  #if DHT_EXIST == 1
    #if DEBUG == 1
      Serial.println("DHT                 init");
    #endif //DEBUG
    dht.begin();
    delay(100);
  #endif //DHT_EXIST

  #if SHT_EXIST == 1
    #if DEBUG == 1
      Serial.println("SHT                 init");
    #endif //DEBUG
  #endif //SHT_EXIST

  #if HTU21_EXIST == 1
    delay(50);

    if (!htu21.begin())
    {
      #if DEBUG == 1
        Serial.println("Error. Couldn't find HTU21!");
      #endif //DEBUG
      htu21_error = true;
    }
    else
    {
      #if DEBUG == 1
        Serial.println("HTU21               init");
      #endif //DEBUG
    }
  #endif //HTU21_EXIST

  #if CCS811_EXIST == 1 // <-------- i2c
    delay(50);

    if(!ccs.begin()){
      #if DEBUG == 1
        Serial.println("Failed to start sensor! Please check your wiring.");
      #endif //DEBUG
      ccs811_error = true;
    }
    else{
      #if DEBUG == 1
        Serial.println("CCS811              init");
      #endif //DEBUG
    }

    // Wait for the sensor to be ready
    while(!ccs.available());
  #endif //CCS811_EXIST


  #if DEBUG == 1
    Serial.println("=============== END OF CONFIG ===============");
    Serial.println("");
  #endif
  delay(100);

  runOnce();
} // setup()

void loop()
{
  #if USE_SLEEP_MODE != 1
    currentUpdateMillis = millis();
    if (currentUpdateMillis - lastUpdateMillis >= updateInterval)
    {
      lastUpdateMillis = currentUpdateMillis;
      update_flag = true;
    }

    // if it's time to update - read sensors, send data
    if (update_flag == true)
    {
      #if DEBUG == 1
        Serial.println("\r\n\n-->start");
        Serial.print("WIFI status code: ");
        Serial.println(WiFi.status());
      #endif //DEBUG

      readSensors();

      #if USELED == 1
        ledBlink(200, 3);
      #endif //USELED

      #if OLED == 1
        displayDraw();
      #endif //OLED

      #if NARODMON == 1
        currentSendMillis = millis();

        if (currentSendMillis - lastSendMillis >= sendPeriod)
        {
          lastSendMillis = currentSendMillis;
          send_allow_flag = true;
        }
        else
        {
          #if DEBUG == 1
            Serial.print("Next delivery in ");
            Serial.print(round((sendPeriod - (currentSendMillis - lastSendMillis)) / 1000));
            Serial.println(" seconds");
          #endif //DEBUG
        }

        // if it's time to send post message - send message
        if (send_allow_flag == true)
        {
          if (!wifi_error)
          {
            send_message(POST_string);
          }
          else
          {
            #if DEBUG == 1
              Serial.println("Cannt POST to server. WiFi error");
            #endif
          }
        }
      #endif //narodmon

      update_flag = false;
      #if DEBUG == 1
        Serial.println("-->end");
      #endif //DEBUG
    }    //if(update_flag == true)
  #endif //!USE_SLEEP_MODE
} // loop()

String floatToString(float src, char decimal_point)
{
  String data = "";
  if (src < 0) //check for minus sign
  {
    data = "-";
  }
  int a, b, c;
  a = (int)abs(round(src * 100));
  b = (int)floor(a / 100);
  c = (int)floor(a % 100);
  data += String(b);
  data += String(decimal_point);

  if (c >= 10)
  {
    data += String(c);
  }
  else
  {
    data += "0";
    data += String(c);
  }
  return data;
} // floatToString()

#if USE_SLEEP_MODE == 1
  void goto_sleep()
  {
    #if DEBUG == 1
      Serial.println("Do nothing for 10 seconds"); // This makes it easier to re-flash the chip later.
    #endif                                         //DEBUG
      delay(10000);

    #if DEBUG == 1
      Serial.print("Off To Sleep for ");
      Serial.print(round(SLEEPING_TIME / 60e6));
      Serial.println(" min");
    #endif //DEBUG

    #if MOSFETSENSORS == 1
      turnSensorsOFF();
    #endif //MOSFETSENSORS

    #if USELED == 1
      ledBlink(400, 3);
    #endif
      ESP.deepSleep(SLEEPING_TIME, WAKE_RF_DEFAULT); //20 sec

    #if DEBUG == 1
      Serial.println("Why im not sleeping?");
    #endif //DEBUG
  } // goto_sleep()
#endif //USE_SLEEP_MODE

#if USELED == 1
  void ledBlink(int duration, int count)
  {
    for (int i = 0; i < count; i++)
    {
      digitalWrite(INDICATORLED, HIGH); // turn the LED on
      delay(duration);
      digitalWrite(INDICATORLED, LOW); // turn the LED off
      delay(duration);
    }
  } // ledBlink()
#endif //USELED

#if NARODMON == 1
void send_message(String data)
{
  #if DEBUG == 1
    Serial.println("=============== send msg start ==============");
  #endif //DEBUG

  #if WIFI == 1
    if (WiFi.status() == WL_CONNECTED)
    {
      WiFiClient client;
      if (!client.connect(host, httpPort))
      {
        #if DEBUG == 1
          Serial.println("-------------- host unreachable -------------");
          Serial.println("               reboot in 10 sec              ");
          Serial.println("---------------------------------------------");
        #endif //DEBUG
        #if USELED == 1
          ledBlink(50, 5);
        #endif
          ESP.deepSleep(10000, WAKE_RF_DEFAULT); //sleep 10 sec
      }

      #if DEBUG == 1
        Serial.println("WIFI:               Connected\n");
      #endif //DEBUG

      client.print(String("POST http://narodmon.ru/post.php HTTP/1.0\r\nHost: narodmon.ru\r\nContent-Type: application/x-www-form-urlencoded\r\n"));
      client.print(String("Content-Length: " + String(data.length()) + "\r\n\r\n" + data + "\r\n\r\n"));

      #if DEBUG == 1
        Serial.println(String("POST http://narodmon.ru/post.php HTTP/1.0\r\nHost: narodmon.ru\r\nContent-Type: application/x-www-form-urlencoded\r\n"));
        Serial.println(String("Content-Length: " + String(data.length()) + "\r\n\r\n" + data + "\r\n\r\n"));
        Serial.println("Server reply begin =====>");
      #endif //DEBUG

      delay(50);
      while (client.available())
      {
        String line = client.readStringUntil('\r');

        #if DEBUG == 1
          Serial.print(line);
          Serial.print(String(line.length()));
        #endif //DEBUG

        #if OLED == 1
          if (!oled_error)
          {
            display.clearDisplay();
            display.setCursor(0, 0);
            display.println(line.substring(0, 20));
            display.display();
          }
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
      #if USELED == 1
        ledBlink(100, 5);
      #endif
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
} // send_message()
#endif //NARODMON
// ###########################

void runOnce()
{
  #if DEBUG == 1
    Serial.println("runOnce:            Going to check sensors");
  #endif //DEBUG

  readSensors();

  #if DEBUG == 1
    Serial.println("runOnce:            Sensors checked");
  #endif //DEBUG

  #if OLED == 1
    displayDraw();
  #endif //oled

  #if USE_SLEEP_MODE == 1
    checkResetInfo(); // send request to server if waked after sleep
  #endif // USE_SLEEP_MODE

  #if USELED == 1
    ledBlink(100, 3);
  #endif // USELED

  #if NARODMON == 1
    POST_string = "";
  #endif //NARODMON

  #if USE_SLEEP_MODE == 1
    goto_sleep();
  #endif //USE_SLEEP_MODE
} // runOnce()

void readSensors()
{
  #if DEBUG == 1
    Serial.println("readSensors:        Going to collect data");
  #endif //DEBUG
  #if NARODMON == 1
    POST_string = "ID=";
    POST_string += narodmonDevId;
  #endif //NARODMON

  #if DALLAS_EXIST == 1
    if (!dallas_error)
    {
      #if DEBUG == 1
        Serial.println("readSensors:        Dallas");
      #endif //DEBUG
      dallas_sensors.requestTemperatures();

      #if NARODMON == 1
        for (int i = 0; i < countOfDallasTerm; i++)
        {
          if (dallas_sensors.getAddress(tempDeviceAddress, i))
          {
            #if DEBUG == 1
              Serial.print(aGetTempAddress(tempDeviceAddress));
              Serial.print(":   ");
              Serial.println(aGetTemperature(tempDeviceAddress));
            #endif //DEBUG

            POST_string += "&";
            POST_string += aGetTempAddress(tempDeviceAddress);
            POST_string += "=";
            POST_string += aGetTemperature(tempDeviceAddress);
          }
        }
      #endif //NARODMON
    }
    else
    {
      #if DEBUG == 1
        Serial.println("readSensors:       DallasError");
      #endif //DEBUG
    }
  #endif //DALLAS_EXIST

  #if BMP_EXIST == 1
    if (!bmp_error)
    {
      #if DEBUG == 1
        Serial.println("readSensors:        BMP180");
      #endif //DEBUG

      presureSensor.getEvent(&event);
      if (event.pressure)
      {
        float presure = 0.0;
        presure = event.pressure;

        #if DEBUG == 1
          Serial.print("bmp180 pressure     ");
          Serial.print(presure);
          Serial.println(" hPa");
        #endif //DEBUG

        #if NARODMON == 1
          POST_string += "&";
          POST_string += narodmonDevId;
          POST_string += "01=";
          POST_string += floatToString(presure);
        #endif //NARODMON

        float temperature = 0.0;
        presureSensor.getTemperature(&temperature);

        #if DEBUG == 1
          Serial.print("bmp180 temperature  ");
          Serial.print(temperature);
          Serial.println(" C");
        #endif // DEBUG

        #if NARODMON == 1
          POST_string += "&";
          POST_string += narodmonDevId;
          POST_string += "03=";
          POST_string += floatToString(temperature);
        #endif //NARODMON
      }
      else
      {
        #if DEBUG == 1
          Serial.println("Sensor error");
        #endif //DEBUG
      }
    }
    else
    {
      #if DEBUG == 1
        Serial.println("BMP180 error");
      #endif //DEBUG
    }
  #endif //BMP_EXIST

  #if BH1750_EXIST == 1
    #if DEBUG == 1
      Serial.println("readSensors:        BH1750");
    #endif //DEBUG
    if (!bh1750_error)
    {
      lux = lightMeter.readLightLevel();
      delay(200);
      lux = lightMeter.readLightLevel();

      #if DEBUG == 1
        Serial.print("BH1750              ");
        Serial.print(lux);
        Serial.println(" lux");
      #endif //DEBUG
      #if NARODMON == 1
        POST_string += "&";
        POST_string += narodmonDevId;
        POST_string += "05=";
        POST_string += floatToString(lux);
      #endif //NARODMON
    }
    else
    {
      #if DEBUG == 1
        Serial.println("BH1750  Error");
      #endif //DEBUG
    }
  #endif //BH1750_EXIST

  #if DHT_EXIST == 1
    float newT = dht.readTemperature();

    if (isnan(newT))
    {
      #if DEBUG == 1
        Serial.println("Failed to read from DHT sensor!");
      #endif //DEBUG
      dht_error = true;
    }
    else
    {
      dht_t = newT;
    }
    // Read Humidity
    float newH = dht.readHumidity();

    if (isnan(newH))
    {
      #if DEBUG == 1
        Serial.println("Failed to read from DHT sensor!");
      #endif //DEBUG
      dht_error = true;
    }
    else
    {
      dht_h = newH;
    }
    if(!dht_error)
    {
      #if DEBUG == 1
        Serial.print("DHT_T: ");
        Serial.print(dht_t);
        Serial.print(" ");
        Serial.print("DHT_H: ");
        Serial.println(dht_h);
      #endif //DEBUG
      #if NARODMON == 1
        POST_string += "&";
        POST_string += narodmonDevId;
        POST_string += "AA=";
        POST_string += floatToString(dht_h);
        POST_string += "&";
        POST_string += narodmonDevId;
        POST_string += "AB=";
        POST_string += floatToString(dht_t);
      #endif //NARODMON
    }
  #endif //DHT_EXIST

  #if SHT_EXIST == 1
    float newT = sht.readTemperatureC();

    if (isnan(newT))
    {
      #if DEBUG == 1
        Serial.println("Failed to read from SHT sensor!");
      #endif //DEBUG
      sht_error = true;
    }
    else
    {
      sht_t_c = newT;
    }
    // Read Humidity
    float newH = sht.readHumidity();

    if (isnan(newH))
    {
      #if DEBUG == 1
        Serial.println("Failed to read from SHT sensor!");
      #endif //DEBUG
      sht_error = true;
    }
    else
    {
      sht_h = newH;
    }
    if(!sht_error)
    {
      #if DEBUG == 1
        Serial.print("SHT_T:              ");
        Serial.print(sht_t_c, DEC);
        Serial.println(" C");
        Serial.print("SHT_H:              ");
        Serial.print(sht_h);
        Serial.println(" %");
      #endif //DEBUG
      #if NARODMON == 1
        POST_string += "&";
        POST_string += narodmonDevId;
        POST_string += "BA=";
        POST_string += floatToString(sht_h);
        POST_string += "&";
        POST_string += narodmonDevId;
        POST_string += "BB=";
        POST_string += floatToString(sht_t_c);
      #endif //NARODMON
    }
  #endif //SHT_EXIST

  #if ANALOG_SENSOR == 1
    #if DEBUG == 1
      Serial.print("readSensors:        Analog sensor: ");
      Serial.println(analogRead(ANALOG_PIN));
    #endif //DEBUG

    #if NARODMON == 1
      POST_string += "&";
      POST_string += narodmonDevId;
      POST_string += "06=";
      POST_string += floatToString(analogRead(ANALOG_PIN));
    #endif //NARODMON
  #endif //ANALOG_SENSOR

  #if MUX_EXIST == 1
    int channel_data = 0;

    for (int mux_channel = 0; mux_channel < 8; mux_channel++)
    {
      #if DEBUG == 1
        Serial.println("Mux channel swithed");
      #endif
      muxSwitchTo(mux_channel);

      channel_data = analogRead(mux_data_line);
      #if DEBUG == 1
        Serial.print("Channel: ");
        Serial.print(mux_channel);
        Serial.print(" Data: ");
        Serial.println(channel_data);
      #endif

      #if NARODMON == 1
        POST_string += "&";
        POST_string += narodmonDevId;
        POST_string += muxPrefix;
        POST_string += (String)mux_channel;
        POST_string += "=";
        POST_string += floatToString(channel_data);

      #endif //NARODMON
      delay(100);
    }
  #endif // MUX_EXIST

  #if HTU21_EXIST == 1
    if (!htu21_error)
    {
      #if DEBUG == 1
        Serial.println("readSensors:        HTU21");
      #endif //DEBUG

      htu21_h = htu21.readHumidity();
      htu21_t = htu21.readTemperature();

      #if DEBUG == 1
        Serial.print("HTU humidity        ");
        Serial.print(htu21_h);
        Serial.println(" +- 2% RH");
        Serial.print("HTU temp            ");
        Serial.print(htu21_t);
        Serial.println(" +- 0.5 deg.C");
      #endif //DEBUG

      #if NARODMON == 1
        POST_string += "&";
        POST_string += narodmonDevId;
        POST_string += "07=";
        POST_string += floatToString(htu21_h);
        POST_string += "&";
        POST_string += narodmonDevId;
        POST_string += "08=";
        POST_string += floatToString(htu21_t);
      #endif //NARODMON
    }
    else
    {
      #if DEBUG == 1
        Serial.print("HTU Error");
      #endif //DEBUG
    }
  #endif //HTU21_EXIST

  #if CCS811_EXIST == 1
    if(!ccs811_error)
    {
      #if DEBUG == 1
        Serial.println("readSensors:       CCS811");
      #endif //DEBUG
      if(ccs.available())
      {
        ccs811_eco2 = ccs.geteCO2();
        ccs811_tvoc = ccs.getTVOC();
      }

      #if DEBUG == 1
        Serial.print("eCO2                ");
        Serial.println(ccs811_eco2);

        Serial.print("TVOC ppm            ");
        Serial.println(ccs811_tvoc);

      #endif //DEBUG

      #if NARODMON == 1
        POST_string += "&";
        POST_string += narodmonDevId;
        POST_string += "09=";
        POST_string += floatToString(ccs811_eco2);
        POST_string += "&";
        POST_string += narodmonDevId;
        POST_string += "10=";
        POST_string += floatToString(ccs811_tvoc);
      #endif //NARODMON
    }
    else
    {
      #if DEBUG == 1
        Serial.println("CCS811 Error");
      #endif //DEBUG
    }
  #endif //CCS811_EXIST

  #if NARODMON == 1
    #if DEBUG == 1
      Serial.println();
      Serial.print("Compiled string: ");
      Serial.println(POST_string);
    #endif //DEBUG
  #endif //NARODMON
  update_flag = false;
} //readSensors

#if OLED == 1
  void displayDraw()
  {
    if (!oled_error)
    {
      display.clearDisplay();
      #if HTU21_EXIST == 1
        if (!htu21_error)
        {
          display.setCursor(0, 32);
          display.print("H:");
          display.print(htu21_h);
          display.setCursor(64, 32);
          display.print("T:");
          display.print(htu21_t);
        }
        else
        {
          display.setCursor(0, 32);
          display.print("H:");
          display.print("Err");
          display.setCursor(64, 32);
          display.print("T:");
          display.print("Err");
        }
      #endif // HTU21_EXIST

      #if BH1750_EXIST == 1
        if (!bh1750_error)
        {
          display.setCursor(0, 48);
          display.print("L:");
          display.print((String)lux);
        }
        else
        {
          display.setCursor(0, 48);
          display.print("L:");
          display.print("Err");
        }
      #endif //BH1750_EXIST

      #if BMP_EXIST == 1
        if (!bmp_error)
        {
          display.setCursor(64, 48);
          display.print("P:");
          display.print(event.pressure);
        }
        else
        {
          display.setCursor(64, 48);
          display.print("P:");
          display.print("Err");
        }
      #endif //BMP_EXIST
      display.display();
      delay(1000);
    } //(!oled_error)
  } //displayDraw()
#endif //OLED

#if USE_SLEEP_MODE == 1
void checkResetInfo()
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
      Serial.print("Wake code:          ");
      Serial.print(resetInfo->reason);
      Serial.print("- ");
    #endif //DEBUG

    //switch reset reason
    switch (resetInfo->reason)
    {
    case 0: // первичная загрузка
      #if DEBUG == 1
        Serial.println("Normal startup");
      #endif //DEBUG
      #if USELED == 1
        ledBlink(500, 2);
      #endif //USELED
            
      break;

    case 1:
      #if DEBUG == 1
        Serial.println("Hardware watch dog reset");
      #endif //DEBUG
      #if USELED == 1
        ledBlink(500, 4);
      #endif //USELED
      break;

    case 2:
      #if DEBUG == 1
        Serial.println("Exception reset, GPIO status won’t change");
      #endif //DEBUG
      #if USELED == 1
        ledBlink(500, 5);
      #endif //USELED
      break;

    case 3:
      #if DEBUG == 1
        Serial.println("Software watch dog reset, GPIO status won’t change");
      #endif //DEBUG
      #if USELED == 1
        ledBlink(500, 5);
      #endif //USELED
      break;

    case 4:
      #if DEBUG == 1
        Serial.println("Software restart ,system_restart , GPIO status won’t change");
      #endif //DEBUG
      #if USELED == 1
        ledBlink(500, 5);
      #endif //USELED
      break;

    case 5: //wake up
      #if USELED == 1
        ledBlink(500, 3);
      #endif //USELED
      #if DEBUG == 1
        Serial.println("Wake up from deep-sleep");
        Serial.println("Preparing request");
      #endif //DEBUG

      #if NARODMON == 1
        if (!wifi_error)
        {
          send_message(POST_string);
        }
        else
        {
          #if DEBUG == 1
            Serial.println("Cant POST data. Wifi error");
          #endif
        }
      #endif //NARODMON
      break;

    case 6: //button reset
      #if DEBUG == 1
        Serial.println("External system reset");
      #endif //DEBUG
      #if USELED == 1
        ledBlink(500, 1);
      #endif //USELED
      break;
    }
  #endif //WIFI
}
#endif //USE_SLEEP_MODE

#if MOSFETSENSORS == 1
  void turnSensorsON()
  {
    digitalWrite(POWERPIN, HIGH);
    #if DEBUG == 1
      Serial.println("Sensors ON");
    #endif
  } // turnSensorsON()

  void turnSensorsOFF()
  {
    digitalWrite(POWERPIN, LOW);
    #if DEBUG == 1
      Serial.println("Sensors OFF");
    #endif
  } // turnSensorsOFF()
#endif //MOSFETSENSORS

#if DALLAS_EXIST == 1
  #if DEBUG == 1
    void printAddress(DeviceAddress da)
    {
      for (uint8_t i = 0; i < 8; i++)
      {
        if (da[i] < 16)
          Serial.print("0");
        Serial.print(da[i], HEX);
      }
    } // printAddress()
  #endif //DEBUG

  String aGetTemperature(DeviceAddress da)
  {
    return floatToString(dallas_sensors.getTempC(da));
  } // aGetTemperature()

  String aGetTempAddress(DeviceAddress da)
  {
    String temps = "";
    for (uint8_t i = 0; i < 8; i++)
    {
      int a, b = 0;
      a = (int)floor(da[i] / 16);
      b = (int)floor(da[i] % 16);
      a < 10 ? temps += (String)a : temps += (String)(char((a - 10) + 'A'));
      b < 10 ? temps += (String)b : temps += (String)(char((b - 10) + 'A'));
    }
    return temps;
  } //aGetTempAddress()
#endif // DALLAS_EXIST

#if BMP_EXIST == 1
  void getBMPsensor(void)
  {
    if (!bmp_error)
    {
      sensor_t bmp085_sensor;
      presureSensor.getSensor(&bmp085_sensor);
    }
  }
#endif //BMP_EXIST

#if MUX_EXIST == 1
  void muxSwitchTo(int which_channel)
  {
    for (int input_pin = 0; input_pin < 3; input_pin++)
    {
      int pin_state = bitRead(which_channel, input_pin);
      // turn the pin on or off:
      digitalWrite(mux_control_pins[input_pin], pin_state);
    }
  }
#endif // MUX_EXIST