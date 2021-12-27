
## Unreleased (development)


## Released

### 0.2.3 (515)
 
  - Fixed SSD1306 libray dependency 
  - Updated README.md and corrected some spelling errors 

### 0.2.2 (514)

  - Added support for user:password authorization when connecting to the MQTT broker
  - Added missing sound field in alerts definition in `devices.cpp`
  - Fixed quotes when building `versionURL` string in `sota.cpp`
  - Updated README.md


### 0.2.1 (513) f335ed0

  - Added sound alarm for flashing alerts. Implemented with an active buzzer and transistor
  - Updated README.md
  - Removed firmware name error used to test errors in loading firmware

### 0.2.0 (512) ce1c5b0
  
  - Added configuration menu for manual OTA firmware updating, downloading of configuration or using of default configuration, etc.
  - Added a default device which is toggled off/on with the push button when the screen is blanked
  - More refactoring of `main.cpp`
  - Experimental "ballistic" rotation handler for the rotary encoder
  - Updated README.md
  

### 0.1.3 (259) e117acb

  - Added over-the-air updating of configuration parameters in `config.cpp`
  - Updated README.md to explain how to update configuration parameters
  - Added a section about initial connections in README.md 
  - Removed extraneous board definition in `[env]` section of `platformio.ini`
  - Cleaned up library dependencies in `platformio.ini`


### 0.1.2 (258) 55998b

  - Updated displayed messages at startup and improved translations 
  - Added a new section to README.md on OTA firware updates
  - Version number in decimal now shown in brackets in this file
  - Added firmware over-the-air self-updating 
  - Removed unused routines in `config.cpp`
  - Added a table of content to README.md
  - Updated `platformio.ini` following the SSD1306 library name change 


### 0.1.1 (257) a34a803

- Added EN/FR language support for text shown on OLED display
- Changed time delay macros to seconds from milliseconds in `config.h`
- Added time delays for displayed messages during setup in `config.h`
- Long button press now clears Wi-Fi credentials to allow connecting to a second wireless network without bringing first one down
- Reorganized layout of functions in `main.cpp`
- Changed the name of the serial port baud to SERIAL_BAUD in `platformio.ini`
- Added this file


### 0.1.0 (256) 0b20d0e

- Initial public release
