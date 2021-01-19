
## Unreleased (development)


## Released


### 0.1.2 (258)

  - Updated displayed messages at startup and improved translations 
  - Added a new section to README.md on OTA firware updates
  - Version number in decimal now shown in brackets in this file
  - Added OTA self-updating of the firmware
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

