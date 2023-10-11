
# IMU Demo V2

Designed for CSU Outreach to demonstrate the capabilities of an inertial measurement unit in real time.

## Parts ##
Part Name      | Link
-------------  | -------------
ESP32 Huzzah   | https://www.adafruit.com/product/3405
LSM9DS1        | https://www.adafruit.com/product/3387
LiPo Battery   | https://www.adafruit.com/product/1578
Battery Charger| https://www.adafruit.com/product/4410

Another thing you might want to consider purchasing is a cheap wifi router, which will be elaborated on further later.

## Setup ##
The main requirement to get the code working is setting up PlatformIO for VSCode. Ardunio's IDE stopped support for writing multiple files to an ESP32, so this is how we get around that.

In theory you shouldn't have to download any extra libraries for any part of this code, because the platform.ini file facilitates that for you.

The only thing outside of PlatformIO that you need to download should drivers for the ESP32 which you can find here: http://www.silabs.com/products/development-tools/software/usb-to-uart-bridge-vcp-drivers

## Editing the Code ##
As it stands currently, the code is very plug and play, the only edit that you want to make is in main.cpp.

Replace with network info on line 38:
```
// Replace with your network credentials
const char* ssid     = "ssid";
const char* password = "password";
```
Network security is of utmost importance, so I would strongly reccommend using your own router and entering in that information instead of a public network.

## Running the code ##
To build and upload the code you'll have to go into the platformIO sidebar, and click on:

-> Build Filesystem image

-> Upload and Monitor

## Accessing the Webserver ##
Once you get to this step you should be successfully running your code. In the serial monitor it should tell you what the ESP32's IP address is.

Log into your router, and go to that IP address. If you did everything correctly you should see your website with realtime data.

