# ESP8266-based Watering System #

## What is this repository for? ##

It's a authomatic watering system based on ESP8266 and Arduino IDE.

Main sysem is based on NodeMCU v2 board, so other ESPs may need any adaption (i.e.: analog input adaptor, as NodeMCU has normalized 3.3V analog input meanwhile other ESPs have 1v input).

Also all libraries used are Arduino-copatible, so should be possible to adapt this program to any kind of Arduino-IDE supported platform, as Arduino Uno + Ethernet shield (not tested, maybe there're resource problems)


## How do I get set up? ##

 * Get the ZIP.
 * Get my uRTCLib library: You can look at Arduino IDE library manager or download it from https://github.com/Naguissa/uRTCLib
 * If you want to use MQTT, get Nick O'Leary PubSub Library (MQTT functionality). It's available on Arduino IDE's Library Manager. Also see http://pubsubclient.knolleary.net/
 * If you want to use DHT sensor, get DHT sensor library by Adafruit and its dependencies. All are available on Arduino IDE's Library Manager:
  - Adafruit Unified Sensor by Adafruit: https://github.com/adafruit/Adafruit_Sensor
  - DHT Sensor Library by Adafruit: https://github.com/adafruit/DHT-sensor-library

 * You can edit .h file to configure watering system with your options (filesystem, connection pins, negate logic devices, etc)
 * Upload sketch to microcontroller
 * Prepare extra data:
  * If you want to use SD-card, format SD-card as FAT and copy files in data/ folder to SD-card.
  * If you want to use SPIFFS instead an SD-card, upload extra data at Arduino IDE -> Tools -> ESP8266 Sketch Data Upload
  * If no logs and extras, simply do nothing (except enable that option in .h file).

## Recommendations ##

As internal data is small, use as small SPIFFS size as possible. On nodeMCU, it's preferred 1Mb SPIFFS / 3Mb code against 1Mb code / 3Mb SPIFFS

You can upload programs using OTA on same minor versions but you will need to update DATA folder when changing among major versions.


## Connections on NodeMCU v3 ##

(piece pin -- NodeMCU v3 pin)

### Common ###

RTC:
 * SDA -- D3
 * SCL -- D4
 * VCC -- 3V 
 * GND -- G
  
Relay:
 * GND and COM joined with a jumper
 * IN -- D1
 * GND -- G
 * VCC -- 3V
  
Water level switch (with VCC and GND):
 * VCC - 3V
 * GND - G
 * NO or NC connected to D2, depending wanted logic

Water level switch (mechanical). Pin uses PULLUP mode:
 * COM - G
 * NO -- D2
 
Water pump:
 * VCC -- relay NO
 * GND -- G

Eletrovalve:
 * Can be done as Water pump or connect as the relay, if it is low power one.


### With SD-card, not compatible with low power mode or DHT11, DHT21 or DHT22 ###

https://github.com/Naguissa/watering/blob/master/doc/watering%20-%20sdcard.svg


SD-Card reader, optional:
 * MISO -- D6
 * SCK -- D5
 * MOSI -- D7
 * CS -- D8
 * 5V -- Not connected
 * 3.3V -- 3V
 * GND -- G

Soil Sensor:
 * AO -- A0 (remember using a voltage divissor if your oard doesn't have it built-in)
 * DO -- Not connected
 * GND -- G
 * VCC - D0 (digital pin here to connect/disconnect sensor as needed)

### Without SD-card, compatible with low power mode and/or DHT11, DHT21 or DHT22 ###

https://github.com/Naguissa/watering/blob/master/doc/watering%20-%20spiffs.svg

* Note: You can use SPIFFS for storage or directly nothing.
  
Soil Sensor:
 * AO -- A0 (remember using a voltage divissor if your board doesn't have it built-in)
 * DO -- Not connected
 * GND -- G
 * VCC -- D5 (digital pin to connect/disconnect sensor as needed)
  
DHT11/21/22:
 * Pin1, VCC -- D7 (Could be D5, shared with Soil Sensor, as follow same opperation pattern)
 * Pin2, DATA -- D6
 * Pin4, GND -- G
 
 * Remember 10K resistor between DHT pin1 and pin2.
 
**Deep Sleep activation**

 * Remember you NEED to connect GPIO16 (D0 on NodeMCUv3) and RST pins on ESP8266 devicein order to be able to wake-up.

## Who do I talk to? ##

 * [Naguissa](https://github.com/Naguissa)
 * Spanish site: http://www.foroelectro.net/proyectos-personales-f23/proyecto-de-riego-automatico-t103.html
 * http://www.naguissa.com
