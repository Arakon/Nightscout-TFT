# Nightscout-TFT
Nightscout-based BG value display based on a 3$ Wemos D1 Mini + 4$ TFT Display

#### Requires the following libraries:

https://github.com/esp8266/Arduino (Follow the README about getting the Additonal Board Manager set up)

https://github.com/bblanchon/ArduinoJson (Version 5.x is required, version 6.x will NOT work!)

https://github.com/adafruit/Adafruit-GFX-Library

https://github.com/PaulStoffregen/Time

https://github.com/Bodmer/JPEGDecoder

https://github.com/Bodmer/TFT_eSPI (after installation, copy my User_Setup.h to libraries/TFT_eSPI)

https://github.com/esp8266/arduino-esp8266fs-plugin SPIFFS for Arduino IDE Plugin, follow the instructions on the readme

![Image](/IMG_20190620_220930.jpg)


Wiring diagram for the screen is included. Wire it up, install the libraries above, copy the User_Setup.h, set up SPIFFS plugin, load the sketch, use SPIFFS plugin to upload Sketch data, edit your SSID/Password/Nightscout website (both heroku and azure should work), compile and flash.
A stl file for a case is also included. Print, stick your circuitry into it, align with the gap for the screen, use hot glue to keep in place, or epoxy.

###### Note that I am not a coder. I pieced this software together from a ton of code examples, experiments, and possibly sheer luck. It's ugly, it's likely overcomplicated, but it does the trick.

#### Links to the parts I used

[Aliexpress link to screen](https://www.aliexpress.com/item/32880846744.html)

[Aliexpress link to Wemos D1 Mini](https://www.aliexpress.com/item/32980161356.html)


The sketch is roughly based on this code: https://github.com/robjohnc/nightscout_wemos_epaper, although, due to the different nature of the TFT display, I had to abandon all but the json decoding part. The TFT display shuts down when the ESP8266 is in deep sleep mode, so it's unfortunately not used and will continuously drain power. Still, it runs ~12h on a 3000 mAh power bank, or just power it from an USB power supply/charger.

### Functions

- The data is loaded from nightscout every 45 seconds. The clock uses the clock data from NS too, so it's also only updated every 45 seconds.
- The "Last Data" text will turn red if the data is more than 15 minutes old.
- The BG value will change to yellow if high, orange if low, and red if critical low. The thresholds can be set in the sketch.
- The User_Config.h is preconfigured for the above display.
