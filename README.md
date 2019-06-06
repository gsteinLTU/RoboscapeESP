# RoboscapeESP
Roboscape firmware for ESP8266/ESP32 boards.

## Usage

### Installing 

Open the .ino file in the Arduino IDE. The `STASSID` and `STAPSK` #defines will need to be given a value that matches your access point. Then simply upload it to the board and hook the pins up.

If you do not have the ESP board available in your Arduino IDE, add `https://dl.espressif.com/dl/package_esp32_index.json, http://arduino.esp8266.com/stable/package_esp8266com_index.json` to the "Additional Board Manager URLs" field in the preferences.

### Pins
TBD

## Current status

* Can connect to wireless network
* Can connect to Netsblox server
* Sends heartbeat to server (TODO: periodically resend)
* Toggle built-in LED with Roboscape LED command (TODO: support external LEDs)
* Drive two motors in both directions at variable speeds
