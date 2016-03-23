/*
 *  This sketch sends data via HTTP POST requests to narodmon.ru service.
 *
 *  ***WORKING DRAFT***
 *
 *  Sensors:
 *           bmp180
 *           dth11
 *           ds18b20
 *
 *
 *  You need to change:
 *                      String MAC = "FFFF0000FFFF"- hw address of device
 *                      const char* ssid = "xxxxx" - name of wifi AP
 *                      const char* password = "yyyyyyy" - password of wifi AP
 *                      const char* host = "94.19.113.221" - ip of narodmon.ru
 *
 */

#include <ESP8266WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>
void printAddress(DeviceAddress);
void printTemperature(DeviceAddress);
String fl2str(float);

//Serial
#define SERIAL_SPEED 115200
//wifi

const char* ssid     = "xxxxx";
const char* password = "yyyyyyy";
const char* host = "94.19.113.221";

String MAC = "FFFF0000FFFF"; //main device mac-address
String POST_string = "";

#define BMP_EXIST 1  
#define DHT_EXIST 1 
#define DALLAS_EXIST 1
#define BH1750_EXIST 1
#define MQ4_EXIST 0

#if BMP_EXIST == 1
  #include <Wire.h>
  #include <Adafruit_Sensor.h>
  #include <Adafruit_BMP085_U.h>
  Adafruit_BMP085_Unified bmp = Adafruit_BMP085_Unified(10085);
#endif

#if BH1750_EXIST == 1
  #include <Wire.h>
  #include <BH1750.h>
  BH1750 lightMeter;
#endif

#if DHT_EXIST == 1
#define DHTLIB_OK 0
#define DHTLIB_ERROR_CHECKSUM -1
#define DHTLIB_ERROR_TIMEOUT -2

#if MQ4_EXIST == 1
int DIGITAL_SENSOR_PIN = 12;
#endif

class dht11
{
public:
    int read(int pin);
    int humidity;
    int temperature;
};

int dht11::read(int pin)
{
    // BUFFER TO RECEIVE
    uint8_t bits[5];
    uint8_t cnt = 7;
    uint8_t idx = 0;
    // EMPTY BUFFER
    for (int i=0; i< 5; i++) bits[i] = 0;
    // REQUEST SAMPLE
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
    delay(18);
    digitalWrite(pin, HIGH);
    delayMicroseconds(40);
    pinMode(pin, INPUT);
    // ACKNOWLEDGE or TIMEOUT
    unsigned int loopCnt = 10000;
    while(digitalRead(pin) == LOW)
          if (loopCnt-- == 0) return DHTLIB_ERROR_TIMEOUT;
    loopCnt = 10000;
    while(digitalRead(pin) == HIGH)
          if (loopCnt-- == 0) return DHTLIB_ERROR_TIMEOUT;
    // READ OUTPUT - 40 BITS => 5 BYTES or TIMEOUT
    for (int i=0; i<40; i++)
    {
          loopCnt = 10000;
          while(digitalRead(pin) == LOW)
                  if (loopCnt-- == 0) return DHTLIB_ERROR_TIMEOUT;
          unsigned long t = micros();
          loopCnt = 10000;
          while(digitalRead(pin) == HIGH)
                  if (loopCnt-- == 0) return DHTLIB_ERROR_TIMEOUT;
          if ((micros() - t) > 40) 
            bits[idx] |= (1 << cnt);
          if (cnt == 0)   // next byte?
          {
                  cnt = 7;    // restart at MSB
                  idx++;      // next byte!
          }
          else cnt--;
    }
          Serial.print("*** debug: dht bites array= ");
          for(int n=0;n<5;n++){
            Serial.print(bits[n]);Serial.print(":");
          }
          Serial.println();
    // as bits[1] and bits[3] are allways zero they are omitted in formulas.
    humidity    = bits[0]; 
    temperature = bits[2]; 
    uint8_t sum = bits[0] + bits[2];  
    if (bits[4] != sum) return DHTLIB_ERROR_CHECKSUM;
    return DHTLIB_OK;
}

dht11 DHT;
#define DHT11_PIN 2
#endif

#if DALLAS_EXIST == 1
  #define ONE_WIRE_BUS 13
  #define TEMPERATURE_PRECISION 12
  OneWire oneWire(ONE_WIRE_BUS);
  DallasTemperature sensors(&oneWire);
  int numberOfDevices; 
  DeviceAddress tempDeviceAddress; 
#endif

void displaySensorDetails(void)
{
  #if BMP_EXIST == 1
    sensor_t sensor;
    bmp.getSensor(&sensor);

  #endif
}

void setup() {
  Serial.begin(SERIAL_SPEED);
  delay(150);
  Serial.println("=================== CONFIG ==================");
  
  // We start by connecting to a WiFi network

  Serial.println();
  Serial.println();
  Serial.print("Connecting to: ");
  Serial.print(ssid);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println(" DONE");  
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("");  

  #if DALLAS_EXIST == 1
    sensors.begin();
    numberOfDevices = sensors.getDeviceCount();
    Serial.print("Locating dallas devices...\t");
    (numberOfDevices>0)?Serial.print(" DONE\n"):Serial.print(" FAIL\n");
    
    Serial.print("Found ");
    Serial.print(numberOfDevices, DEC);
    Serial.println(" devices.");
    Serial.print("Parasite power is: "); 
    if (sensors.isParasitePowerMode()) Serial.println("ON");
    else Serial.println("OFF");
    
    // Loop through each device, print out address
    for(int i=0;i<numberOfDevices; i++)
    {
        // Search the wire for address
        if(sensors.getAddress(tempDeviceAddress, i))
      {
        Serial.print("Found device ");
        Serial.print(i, DEC);
        Serial.print(" with address: ");
        printAddress(tempDeviceAddress);
        Serial.println();
        
        Serial.print("Setting resolution to ");
        Serial.println(TEMPERATURE_PRECISION,DEC);
        
        // set the resolution to 9 bit (Each Dallas/Maxim device is capable of several different resolutions)
        sensors.setResolution(tempDeviceAddress, TEMPERATURE_PRECISION);
        
         Serial.print("Resolution actually set to: ");
        Serial.print(sensors.getResolution(tempDeviceAddress), DEC); 
        Serial.println();
      }else
      {
        Serial.print("Found ghost device at ");
        Serial.print(i, DEC);
        Serial.print(" but could not detect address. Check power and cabling");
      }
    }

  #endif
  
  #if BH1750_EXIST == 1
    lightMeter.begin();
  #endif

  #if BMP_EXIST == 1
    Wire.begin(4,5);
    if(!bmp.begin())
    {
      Serial.print("No BMP180 detected.");
      //while(1);
    }
    displaySensorDetails();
  #endif  

#if MQ4_EXIST == 1
  pinMode(DIGITAL_SENSOR_PIN, INPUT);
#endif
  
 Serial.println("=============== END OF CONFIG ===============");
 Serial.println("");
}

void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}

#if DALLAS_EXIST == 1
  String aGetTempAddress(DeviceAddress deviceAddress)
  {
    String temps="&";
    for (uint8_t i = 0; i < 8; i++)
    {
      int a,b =0;
      a =(int)floor(deviceAddress[i]/16);
      b= (int)floor(deviceAddress[i]%16);
      a<10?temps+=(String)a:temps+=(String)(char((a-10)+'A'));
      b<10?temps+=(String)b:temps+=(String)(char((b-10)+'A'));
    }
    temps+="=";  
    return temps;
  }
 
  void printTemperature(DeviceAddress deviceAddress)
  {
      float tempC = sensors.getTempC(deviceAddress);
      Serial.print("dallas.Temperature: \t");
      Serial.print(tempC);
      Serial.println(" C");
      String temp_string = "";
  }
 
  String aGetTemperature(DeviceAddress deviceAddress)
  {
      float tempC = sensors.getTempC(deviceAddress);
      return fl2str(tempC);
  }
#endif

String fl2str(float src){
  String temp = "";
  String sign = "";
  if(src<0){
    sign= "-";
    src = abs(src);
  }
  
  int a,b,c=0;
  a=(int)round(src*100);
  b=(int)floor(a/100);
  c=(int)floor(a%100);
  temp+= sign;
  temp+= String(b);
  temp+= ".";
  temp+= String(c);
  return temp;
}

void loop() {
  delay(600000);
  //++value;
  Serial.print("Millis: \t"); Serial.println(millis());
  Serial.print("connecting to \t");
  Serial.println(host);
  
  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) 
  {
    Serial.println("connection failed");
    return;
  }

  POST_string = "ID=";
  POST_string += MAC;
  
  #if DALLAS_EXIST == 1
    Serial.print("Requesting temperatures...\t");
    sensors.requestTemperatures(); // Send the command to get temperatures
    Serial.println("DONE");

    for(int i=0;i<numberOfDevices; i++)
    {
      if(sensors.getAddress(tempDeviceAddress, i))
      {
        POST_string +=aGetTempAddress(tempDeviceAddress);
        POST_string +=aGetTemperature(tempDeviceAddress);
      } 
    }
    
  #endif

#if BMP_EXIST == 1
  sensors_event_t event;
  bmp.getEvent(&event);
  
  if (event.pressure)
  {
    POST_string += "&";
    POST_string += MAC;
    POST_string += "01=";
    POST_string += fl2str(event.pressure);
    Serial.print("bmp180.Pressure: \t");         Serial.print(event.pressure);   Serial.println(" hPa");
    float temperature;
    bmp.getTemperature(&temperature);
    Serial.print("bmp180.Temperature: \t");      Serial.print(temperature);      Serial.println(" C");
    POST_string += "&";
    POST_string += MAC;
    POST_string += "03=";
    POST_string += fl2str(temperature);
  }
  else
  {
    Serial.println("Sensor error");
  }
#endif

#if BH1750_EXIST == 1
 uint16_t lux = lightMeter.readLightLevel(); //54000

    POST_string += "&";
    POST_string += MAC;
    POST_string += "05=";
    POST_string += (String)lux;
#endif

#if DHT_EXIST == 1
  int chk;
  chk = DHT.read(DHT11_PIN);    // READ DATA
  Serial.print("dht11.Humidity: \t");        Serial.print(DHT.humidity,1);      Serial.println(" %");
  Serial.print("dht11.Temperature: \t");     Serial.print(DHT.temperature,1);     Serial.println(" C");
  POST_string += "&";
  POST_string += MAC;
  POST_string += "02=";
  POST_string += fl2str(DHT.humidity);
  POST_string += "&";
  POST_string += MAC;
  POST_string += "04=";
  POST_string += fl2str(DHT.temperature);
#endif

#if MQ4_EXIST == 1
  int d_sensor_val = digitalRead(DIGITAL_SENSOR_PIN);
  POST_string += "&";
  POST_string += MAC;
  POST_string += "06=";
  POST_string += fl2str(DHT.humidity);
#endif

  Serial.println();

  Serial.print("Requesting URL: ");
  Serial.println(POST_string);

  client.print(String("POST http://narodmon.ru/post.php HTTP/1.0\r\nHost: narodmon.ru\r\nContent-Type: application/x-www-form-urlencoded\r\n"));
  client.print(String("Content-Length: " + String(POST_string.length()) + "\r\n\r\n" + POST_string + "\r\n\r\n"));

  delay(10);
  
  // Read all the lines of the reply from server and print them to Serial
  Serial.println("server reply =====> ");
  Serial.println("");
  
  while(client.available()){
    String line = client.readStringUntil('\r');
    
    Serial.print(line);
  }
  
  Serial.println("server reply <===== ");
  Serial.println();
  Serial.println("closing connection");

   Serial.println("=============== ITERATION DONE ==============");
   
   Serial.println();
}
