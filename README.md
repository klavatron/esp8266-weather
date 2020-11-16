# esp8266-weather
Esp8266 based weather station for narodmon.ru or other webservices

This is a rough sketch of home weather station based on esp8266-201, esp8866-12f or LoLin NodeMCU module. It gets data from sensors and sends to narodmon.ru.

### Connect 3k resistor between RST and GPIO16/D0 to enable ESP8266 wakeup from deepsleep

## NodeMCU board
![alt text](https://github.com/klavatron/esp8266-weather/blob/master/pcbs/breadboard/weather-st2.jpg)

### Parts used:

- Esp8266 esp-12e LoLin NodeMCU
- Breadboard
- bmp180 - barometer and thermometer
- BH1750 - Digital Light Sensor
- HTU21 - Humidity Sensor
- LED + resistor
- wires


![alt text](https://github.com/klavatron/esp8266-weather/blob/master/pcbs/breadboard/weather-st.png)

### Parts used:

- Esp8266-201 with antenna
- Breadboard
- bmp180 - barometer and thermometer
- dth11 - humidity and thermometer
- ds18b20 - termometer
- 4.7k and 10k resistors
- 3.3v power supply
- wires

![alt text](https://github.com/klavatron/esp8266-weather/blob/master/pcbs/esp201/1.jpg)

### Parts used:

- Esp8266-201 with antenna
- bmp180 - barometer and thermometer
- BH1750 - Digital Light Sensor
- HTU21 - Humidity Sensor
- ds18b20 - termometers
- Capacitive soil moisture sensor
- 7333 LDO 3.3v
- resistors, capacitors

![alt text](https://github.com/klavatron/esp8266-weather/blob/master/pcbs/analog_multiplexer/1.jpg)
  Plants monitor
### Parts used:

- Esp8266-12F
- 7 Capacitive soil moisture sensors
- 7333 LDO 3.3v
- dth11 - humidity and thermometer
- resistors, capacitors
