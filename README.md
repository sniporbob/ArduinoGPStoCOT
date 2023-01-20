# ArduinoGPStoCOT
Arduino GPS to CoT Sketch

This sketch obtains the current GPS position via a serial GPS, creates a Cursor-on-Target XML message, and multicasts the message out via an Ethernet Shield.

Minor configuration of the code will be required. These are:

* deviceName: The name that will show up in ATAK/WinTAK for this device.
* type2525: The icon type that will show up in ATAK/WinTAK for this device (see sketch for additional info).
* mac: The MAC address for the ethernet shield. Typically this is on a sticker affixed to the shield.
* lanIP: A static IP address for the ethernet shield. IP must be within the valid address range for the network this device will be used on.

These are configured at the top of the sketch within a clearly marked area. Additional optional configuration is available
for further customization. This section is immediately following the required configuration.

The code is mostly straightforward. I did my best to add extensive comments.

# Hardware
The sketch was created for the following hardware:

* Arduino Uno
* Arduino Ethernet Shield
* NEO-6M GPS module (from Amazon: https://www.amazon.com/gp/product/B07P8YMVNT)

# Libraries
The sketch requires you to have the following library installed:

* TinyGPSPlus 1.0.3 (most current version at this time)

# Wiring Diagram
Attach the Ethernet Shield to the top of the Arduino Uno like normal. Wire the GPS module to pins 2 and 3 as depicted below (pins can easily be changed in the sketch if needed).
![alt text](https://raw.githubusercontent.com/sniporbob/ArduinoGPStoCOT/main/Wiring%20diagram/GPStoCOTwiring.png)

# Why Would I Want This?
I have no idea. Perhaps it would be useful for attaching to a piece of equipment or a vehicle that you'd like to track, but don't want to dedicate an entire EUD?
This could also form the beginning of a tripwire by incorporating a PIR sensor and multicasting a hostile marker when triggered.
