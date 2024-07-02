
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

### File Directory ###
I've kept in the added the default platformIO files, however, the aren't used so you can ignore them. These are the important directories.

Folder         | Contains
-------------  | -------------
data           | html, js, css files (web server files)
src            | main.cpp (main arduino file)

Additionally, platform.ini saves any configs that you would need to do.

## Setup ##
The main requirement to get the code working is setting up PlatformIO for VSCode. Ardunio's IDE stopped support for writing multiple files to an ESP32, so this is how we get around that.

In theory you shouldn't have to download any extra libraries for any part of this code, because the platform.ini file facilitates that for you.

You have the option of adding your own 3D model to be visualized, make sure that you edit the name of the file in script.js. (It should be roughly on line 86)

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

## Troubleshooting ##
- Make sure when you're uploading the code, that you don't have the ports open or else you won't be able to access them.
- If for whatever reason you need to change filenames make sure you change any references in all the files
- The ESP32 that I use has 4MB of flash memory which means that any 3D model has to be simplified to fit. Removing chamfers can sometimes save you up to 500kb of data.
