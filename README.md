# Domoticz Button

A [PlatformIO](https://platformio.org) project built on an ESP8266 development module (such as a D1 Mini or NodeMCU), 
an SSD1306 128x64 OLED display, and a KY040 rotary encoder with push-button switch that provides a physical interface 
to a home automation system based on [Domoticz](https://domoticz.com). 

# Inspiration

This is based on a similar project, [Kitchen Button](https://github.com/crankyoldgit/Kitchen-Button) by David Conran,
(crankyoldgit) that uses the same hardware to control [Tasmota](https://github.com/arendst/Tasmota) switches directly. It is a great project and it
works as is. But my home automation system has some devices that are not based on Tasmota which I also wanted to control.
Furthermore, there are scenes and groups defined in Domoticz which are quite useful.

# Requirements

There is one requirement on the Domoticz server. The `MQTT Client Gatewway with LAN interface` hardware
has to be set up in Domoticz and its `Publish Topic` field has to be set to `out`.

The following libraries are used

    ESP8266_SSD1306
    WifiManager
    PubSubClient
    bblanchon/ArduinoJson
    https://github.com/sigmdel/mdPushButton.git
    https://github.com/sigmdel/mdRotaryEncoder.git  

Of course a different SSD1306 library could be used but ESP8266_SSD1306 by Daniel Eichhorn with contributions by Fabrice Weinberg has very legible for the complete Latin 1 code page which is quite useful for me. I rolled my own button and encoder libraries, but it should be quite easy to replace them if desired.

# Usage

The user interface consists of a small display and a rotary button with push-button action. Using such a
simple device must be necessarily simple. 

## Display

Even if it is less than an inch (2.54 cm) across, the OLED display is very legible at various angles. The display
is divided into three lines or rows. Here is a virtual device in Domoticz that controls a bedroom light.

    +-----------------+
    |     Bedroom     |  TOP_ROW    contains the device location or zone
    |  Ceiling Light  |  MIDDLE_ROW contains the device name
    |      Off        |  BOTTOM_ROW contains the device status, if it has a status
    +-----------------+

Here is a dimmer with brightness set at 70%. 

    +-----------------+
    |     Kitchen     |
    |  Ceiling Light  |
    |     On (70%)    |
    +-----------------+

Domoticz scenes do not have a status.

    +-----------------+
    |     Bedroom     |
    |    Goodnight    |
    |                 |
    +-----------------+

## Encoder/push-button

Rotating the encoder clockwise or counterclockwise will show the status of the next or previous device. 

Pressing the encoder push-button once will change the status and thus toggle the corresponding IoT device. A lamp will
be turned off or on. A dimmer will be turned off or on without changing its brightness level, at least
in my experience with Tasmota controlled dimmers. A Domoticz scene will be executed. The Goodnight scene
turns on bedside lights and gradually turns off all other lights in the house. This is all defined in 
Domoticz.

If the device is a dimmer, pressing the push-button twice will put the controller in a brightness editing mode.  

    +-----------------+
    |     Kitchen     |
    |  Ceiling Light  |
    |      < 70 >     |
    +-----------------+

Turning the rotary encoder changes the brightness level by 10% at each step. 
Pressing the push-button once sets the dimmer at the shown level and leaves the brightness editing mode. 
Pressing the push-button twice exits the brightness editing mode directly without changing the device.

Domoticz also has selector switches which select one value from a given number of possibilities. 
These are edited in the same manner as dimmer brightness is.

## Display Blanking

After 15 seconds of inactivity, the display is blanked. Pressing the push-button or turning the rotary encoder one
step restores the display. That initial wake up button press or encoder step is ignored.

## Alerts

When the display is blanked, alerts can be flashed (3 seconds on / 3 seconds off). In this example there are two
alerts associated with the garage door. The first shows when the garage door is open. The second shows when
automatic garage door closing has been disabled. Of course if automatic garage door closing is activated, then the first
alert will no longer be shown once the automation kicks in, assuming nothing impeded the progress of the door.

# Setup

Unlike some web interfaces to Domoticz, the Domoticz Button will not obtain a list of devices from the home automation server. The content of `devices.h` and `devices.cpp` must be modified to point to the Domoticz virtual devices that are to be controlled by the button.

## Zones

First define the zones in the house to which devices belong. The enumerated type is defined in the `devices.h` header file.

    // location or room in house in which devices are grouped
    enum zone_t { 
      Z_TOP_FLOOR, 
      Z_GROUND_FLOOR, 
      Z_BASEMENT, 
      Z_GARAGE,
      Z_HOUSE
    };

My zones correspond to the floors in the house, but you may prefer to use
bedrooms or a mixture of both. It does not matter, but remember that these zones correspond to the top row in the display.

Adjust their names in the `zones[]` array of string in `devices.cpp`. Don't forget that the width of the display is limited.

## Device Type

Second adjust the types of device status messages that can occur in `devices.h`.

    // primary device status
    enum devstatus_t { 
        DS_NONE,             // scenes have no status
        DS_OFF, DS_ON,       // lights, dimmers and groups when all devices either on or off
        DS_MIXED,            // groups when some devices on and some device off
        DS_NO, DS_YES,       // automatic garage closing device
        DS_CLOSED, DS_OPEN,  // garage door state
        DS_DEFAULT, DS_WEEKEND, DS_HOLIDAYS // calendar
    }; 

The first three are found in all systems. Since I have groups in Domoticz,
the `DS_MIXED` status is included. It indicates that some devices in a group are on while others are off. The other four status correspond to the automatic garage door closer and the garage door state already mentioned above. The last three are possible values of a selector switch that is used to select a scheduling calendar in Domoticz.

The string representations of all these status is in the `devicestatus[]` array of strings. These are displayed on the third line of the OLED display.

## List of Devices

The "hard work" now begins. A list of all Domoticz devices to be controlled by the button must be set up in `devices.cpp`. This is the `devices[]` array of `device_t` structures.

    typedef struct {
        devstatus_t status;   // primary device status
        int32_t xstatus;      // extra status value such as dim level
        const uint32_t idx;   // Domoticz idx of device
        const devtype_t type; // device type
        const zone_t zone;    // zone in house where device is found
        const char* name;     // name of the device, can be different from that used in Domoticz
    } device_t;

The `status` is updated by the application, just put a reasonable value in the
first field. The extra status, `xstatus`, is usually the brightness level of a dimmer switch or the current selection of a selector switch. Again this will be updated by the 
application, so an intial value of 0 is fine.  The `idx` field is the Domoticz idx for a device. The
device type and zone should be self-explanatory. The last field is the device
name. It will be shown in the middle row of the display. As can be seen, 
14-letter names can be shown with the chosen font.

Here is part of the current definition 

    device_t devices[] {
        /* 00 */   {DS_OFF,   0,   5, DT_SWITCH,   Z_TOP_FLOOR, "Lampe Alice"},
        /* 01 */   {DS_OFF,   0,   6, DT_SWITCH,   Z_TOP_FLOOR, "Lampe Michel"},
        /* 02 */   {DS_NONE,  0,   5, DT_GROUP,    Z_TOP_FLOOR, "Lampes de chevet"},
        ...
        /* 15 */   {DS_NO,    0,  37, DT_SELECTOR, Z_GARAGE, "Fermeture auto."},
        /* 16 */   {DS_OPEN,  0,  29, DT_CONTACT,  Z_GARAGE, "Porte"},
        /* 17 */   {DS_NONE,  0,  28, DT_PUSH_OFF, Z_GARAGE, "Fermer porte"},
        ...
        /* 24 */   {DS_DEFAULT,  0,   159, DT_SELECTOR, Z_HOUSE, "Calendrier"}
    };
 
I suggest starting with on/off switches and dimmers. Add selector switches, groups and 
alerts once the basics are working. 

## Selector Switches

The extra information needed for each selector switch is defined in the `selectors[]` array in `devices.cpp`. The `selector_t` structure must be filled for each selector.

    typedef struct {
        uint16_t index;        // index of the selecton in devices[]
        devstatus_t status0;   // the first possible choice (value 0) 
        uint8_t statusCount;   // the number of choices so the last value is (statusCount-1)*10
    } selector_t;

Note that the first field is the selector switch index in the `devices[]` not the Domoticz idx of the switch. That explains why the indices are shown in comments in the `devices` array. To show the correct status of a selector switch, the enumerated `devtype_t` of its first possible value must be specified in the `status0` field.  To correctly edit this value, the number selector choices or status must be specified in the `statusCount`.

    // Selectors. Currently there are two selectors in the table
    selector_t selectors[] {
        {15, DS_NO, 2},      //Fermeture automatique du garage
        {24, DS_DEFAULT, 3}  //Calendrier
    };

For example, the scheduling calendar selector has three possible values:
`DS_DEFAULT`, `DS_WEEKEND` and `DS_HOLIDAYS`, so its `status0` field is set to `DS_DEFAULT` and that way when editing that extra value, the application knowns what to display for the three possible selection values.

## Groups

Unfortunately, Domoticz does not send an MQTT message to update the status of a group when the status of a member device changes. So the program does it on its own. In order to do that it must know which devices belong to the group.

    typedef struct {
        uint16_t index;       // device index of group 
        uint16_t count;       // number of members in the gourp (max 5)
        uint16_t members[5];  // list of devices index of members of the group
    } group_t;

Again the first field is the group index in the `devices[]` not the Domoticz idx of the group. The second field is the number of member devices and finally there's a list of the indices of the member devices. Currently the limit on the number of device is set at 5. 

## Alerts

Alerts are simply defined by an index in the `devices[]` array identifying the device that can raise an alert and a condition which is nothing else than its status.

    typedef struct {
        const uint16_t index;    // index of device in devices
        const uint8_t condition; // status value that warrants an alert
    } alert_t;

For example, an alert is raised when automatic garage door closing is disabled. The virtual Domoticz device for this is a selector switch and the disable setting is the first selector level which is 0. So the `alert_t` structure for that alert is `{15,0}`.

# Config.h

A number of parameters can be set in the `config.h` header file. Currently, these
are 
   1. The IP address of the MQTT host and the MQTT port. 
   2. The IP address of the syslog host and the syslog port.
   3. The logging level for the UART log sent to the ESP32 Serial port and the system log sent to the syslog host. 
   4. The display timeout interval.
   5. The alert on and off flash interval.

While fields for the MQTT user and password are in place, secure connections for MQTT messages is not yet implemented. Similarly the host name can be specified but this has currently no consequence.

# Licence

The **BSD Zero Clause** ([SPDX](https://spdx.dev/): [0BSD](https://spdx.org/licenses/0BSD.html)) licence applies.




