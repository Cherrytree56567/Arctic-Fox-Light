# Minecraft Arctic Fox Lamp
A simple Minecraft Themed Arctic Fox Lamp that features an ESP32 C6 Mini with ESP Matter Support which supports Temperature, Hue/Sat and Brightness.

## Features
 - Custom Color LED
 - Smart Home Integration
 - Matter Support
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

Then build the project by running
```
idf.py build
```

## Running
To run the application, run:
```
idf.py flash
```

## Credits
 - [Template ESP32 Matter Repo](https://github.com/oidebrett/starter-esp-matter-app)