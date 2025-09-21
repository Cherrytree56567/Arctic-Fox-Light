# Minecraft Arctic Fox Lamp
A simple lamp with a custom enclosure and pcb which allows users to change the color of the led via Smart Home Devices.

## Features
 - Custom Color LED
 - Smart Home Integration
 - Enclosure

## Build
To Run the Matter ESP32 Program, install `ESP-IDF`:
```
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
git submodule update --init --recursive
./install.sh
cd ../
```
Install `ESP-Matter`:
```
git clone https://github.com/espressif/esp-matter.git
cd esp-matter
git submodule update
cd connectedhomeip/connedtedhomeip/
git submodule update
cd ../../
```
Next source both repo's:
```
cd esp-idf
source ./export.sh
cd ../
cd esp-matter
source ./export.sh
cd ../
```
Then 

## Credits
 - [Template ESP32 Matter Repo](https://github.com/oidebrett/starter-esp-matter-app)