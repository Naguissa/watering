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


## Who do I talk to? ##

 * [Naguissa](https://github.com/Naguissa)
 * Spanish site: http://www.foroelectro.net/proyectos-personales-f23/proyecto-de-riego-automatico-t103.html
 * http://www.naguissa.com
