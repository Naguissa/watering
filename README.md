# ESP8266-based Watering System #

## What is this repository for? ##

It's a authomatic watering system based on ESP8266 and Arduino IDE.

Main sysem is based on NodeMCU v2 board, so other ESPs may need any adaption (i.e.: analog input adaptor, as NodeMCU has normalized 3.3V analog input meanwhile other ESPs have 1v input).

Also all libraries used are Arduino-copatible, so should be possible to adapt this program to any kind of Arduino-IDE supported platform, as Arduino Uno + Ethernet shield (not tested, maybe there're resource problems)


## How do I get set up? ##

 * Get the ZIP.
 * Get my uRTCLib library: You can look at Arduino IDE library manager or download it from https://github.com/Naguissa/uRTCLib
 * You can edit .h file to change RTC I2C addresses and configure watering system with your options (filesystem, connection pins, negate logic devices, etc)
 * Upload sketch to microcontroller
 * Prepare extra data:
  * If you want to use SD-card, format SD-card as FAT and copy files in data/ folder to SD-card.
  * If you want to use SPIFFS instead an SD-card, upload extra data at Arduino IDE -> Tools -> ESP8266 Sketch Data Upload
  * If no logs and extras, simply do nothing (except enable that option in sketch).

## Connections on NodeMCU v3 ##

(piece pin -- NodeMCU v3 pin)

 * RTC:
  * SDA -- D3
  * SCL -- D4
  * VCC -- 3V 
  * GND -- G
  
 * SD-Card reader, optional:
  * MISO -- D6
  * SCK -- D5
  * MOSI -- D7
  * CS -- D8
  * 5V -- UNCONNECTED!
  * 3.3V -- 3V
  * GND -- G
  
 * Relay:
  * GND and COM joined with a jumper
  * IN -- D1
  * GND -- G
  * VCC -- 3V
  
 * Soil Sensor:
  * AO -- A0 (remember using a voltage divissor if your oard doesn't have it built-in)
  * DO -- Not connected
  * GND -- G
  * VCC - D0 (digital pin here to connect/disconnect sensor as needed)

 * Water level switch (with VCC and GND):
  * VCC - 3V
  * GND - G
  * NO or NC connected to D2, depending wanted logic

 * Water level switch (mechanical). Pin uses PULLUP mode:
  * COM - G
  * NO -- D2

 * Water pump:
  * VCC -- relay NO
  * GND -- G

 * Eletrovalble:
  * Can be done as Water pump or connect as the relay, if it is low power one.

## Who do I talk to? ##

 * [Naguissa](https://github.com/Naguissa)
 * Spanish site: http://www.foroelectro.net/proyectos-personales-f23/proyecto-de-riego-automatico-t103.html
 * http://www.naguissa.com
