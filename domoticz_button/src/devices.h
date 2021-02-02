#ifndef DEVICES_H
#define DEVICES_H

#include <Arduino.h>

/* * * IoT device * * */

// Types of Domoticz virtual devices that will be handled 
enum devtype_t { 
   DT_SWITCH,   // status is On or OFF
   DT_DIMMER,   // status is On or Off and dim level
   DT_CONTACT,  // status is Closed or Open
   DT_SELECTOR, // status is value of selection};    
   DT_GROUP,    // status is On, Off or mixed depending on status of members
   DT_PUSH_OFF, // no status
   DT_SCENE     // no status
};

// name of each device type - used for logging only
extern const char* devicetypes[];

// location or room in house in which devices are grouped
enum zone_t { 
  Z_TOP_FLOOR, 
  Z_GROUND_FLOOR, 
  Z_BASEMENT, 
  Z_GARAGE,
  Z_HOUSE
};

// names of each zone printed on display
extern const char* zones[];

// primary device status
enum devstatus_t { 
    DS_NONE,             // scenes have no status
    DS_OFF, DS_ON,       // lights, dimmers and groups when all devices either on or off
    DS_MIXED,            // groups when some devices on and some device off
    DS_NO, DS_YES,       // automatic garage closing device
    DS_CLOSED, DS_OPEN,  // garage door state
    DS_DEFAULT, DS_WEEKEND, DS_HOLIDAYS // calendar
}; 

extern const char* devicestatus[]; 


// Structure used to represent each Domoticz virtual device
//
typedef struct {
  devstatus_t status;   // primary device status
  int32_t xstatus;      // extra status value such as dim level
  const uint32_t idx;   // Domoticz idx of device
  const devtype_t type; // device type
  const zone_t zone;    // zone in house where device is found
  const char* name;     // name of the device, can be different from that used in 
                        // domoticz but that would be confusing 
} device_t;

extern device_t devices[];
extern const uint16_t deviceCount; 

// Returns the index of a Domoticz device in the devices[ ] array using
// the given Domoticz idx and the give device type as search criteria.
// The Domoticz idx is unique only for a given type of device,
// so there can be an On/Off switch with idx 6 and a scene with idx 6.
int findDevice(devtype_t type, uint32_t idx);

#ifdef BALLISTIC_ROTATION
extern int nextZone(int cdev);
extern int prevZone(int cdev);
#endif

// Selectors can have many choices 
//
typedef struct {
  uint16_t index;        // index of the selection in devices[]
  devstatus_t status0;   // the first possible choice (value 0) 
  uint8_t statusCount;   // the number of choices so the last value is (statusCount-1)*10
} selector_t;

extern selector_t selectors[];
extern const uint16_t seclectorCount;

// find the index of a selector in the selectors[] array using the
// index in the devices[] array as search criterion.
int findSelector(int index);

// Groups
//
// when there's a change in the  status of a device that is a member
// of a group, Domoticz does not publish an MQTT message reflecting
// any change in the group status. We will have to figure out
// that groupd status ourselves (it can be obained with 
// getsceneinfo but...

typedef struct {
  uint16_t index;       // device index of group 
  uint16_t count;       // number of members in the group (max 5)
  uint16_t members[5];  // list of devices index of members of the group
} group_t;

// Currently two groups in the table
extern group_t groups[];
extern const uint16_t groupCount; 

// Alerts
//

typedef struct {
  uint16_t index;    // index of device in devices
  uint8_t condition; // status value that warrants an alert
  uint8_t sound;     // 0 silent alert, 1 buzzer sounds when alert shown   
} alert_t;


extern alert_t alerts[];
extern const uint16_t alertCount;

extern int currentAlert;

// Returns the index of the next active alert in the alerts[] array.
// If no devices are in an alert condition, returns -1.
// The returned value is also stored in currentAlert
//
int nextAlert(void);

#endif
