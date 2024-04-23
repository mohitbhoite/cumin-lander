# Cumin Lander

This is a firmware repository for the Cumin Lander based on the Seeedstudio XIAO nRF52840. You can find the documentation for this project on my website [here.]( https://www.bhoite.com/sculptures/cumin-lander/)

![Cumin Lander](https://github.com/mohitbhoite/cumin-lander/blob/main/cumin-lander-expanded.png)

## Required libraries:

You'll need to install [Arduino BSP for Adafruit Bluefruit nRF52 series](https://github.com/adafruit/Adafruit_nRF52_Arduino/tree/master) first. Then make sure you have the following libraries.

 - BLE: [Adafruit Bluefruit](https://github.com/adafruit/Adafruit_BluefruitLE_nRF51)
 - Sensor: [Adafruit BME280](https://github.com/adafruit/Adafruit_BME280_Library)
 - Display: [Adafruit SSD1306](https://github.com/adafruit/Adafruit_SSD1306)
 - Graphics: [Adafruit GFX](https://github.com/adafruit/Adafruit-GFX-Library)
 - Memory: [Adafruit Flashtransport](https://github.com/adafruit/Adafruit_SPIFlash/tree/master)



You'll need to install the above libraries on your Arduino IDE first. I used the Arduino IDE v1.8.13 to compile and flash the code on to the nRF52840. Other versions may or may not work.
