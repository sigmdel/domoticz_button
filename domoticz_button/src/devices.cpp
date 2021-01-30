#include <Arduino.h>
#include "devices.h"
#include "logging.h"
#include "lang.h"


const char * zones[] = {
  SC_ZONE0,  // "Upstairs"
  SC_ZONE1,  // "Ground level"
  SC_ZONE2,  // "Downstairs"
  SC_ZONE3,  // "Garage"
  SC_ZONE4   // "House"
};

const char * devicestatus[] = {
  "",                     // "": no status for scenes and push (on/off) buttons
  SC_STATUS_OFF,          // "Off": buttons, dimmers and groups
  SC_STATUS_ON,           // "On":  buttons, dimmers and groups
  SC_STATUS_MIXED,        // "Mixed": groups only
  SC_STATUS_NO,           // "No": automatic garage door closing (selector switch)
  SC_STATUS_YES,          // "Yes": automatic garage door closing (selector switch)
  SC_STATUS_CLOSED,       // "Closed": garage door (contact switch)
  SC_STATUS_OPEN,         // "Open": garage goor (contact switch)
  SC_STATUS_TIMER_PLAN0,  // "Default": normal (in house) timer plan (selector switch)
  SC_STATUS_TIMER_PLAN1,  // "Weekend": weekend (away) timer plan (selector switch)
  SC_STATUS_TIMER_PLAN2   // "Holidays": holidays (away) timer plan (selector switch)
};


// only used in logging messages - not translated
const char * devicetypes[] = {"switch", "dimmer", "push off", "contact", "selector", "group", "scene"};

// Everything in a device_t struct is constant except for on and nvalue which are
// automatically updated from MQTT messages published by domoticz to the 
// domoticz/out topic.


// List of IoT devices that are part of a home automation system based on 
// Domoticz that will be visible with this button. Because the status
// of the devices is constantly being updated, it seems best leave the
// table in RAM. However, if RAM gets short, then the array could be 
// broken up into two parts, keeping all the device constants in an
// array stored in flash memory and keeping the on and nvalue in 
// an array stored  in RAM.

// This is the order in which devices will be displayed. The devices
// are grouped by the room or zone in which they are located

device_t devices[] {
  //                    x=xstatus
  //                    x
  //          statu     x  idx     type        zone        name
  /* 00 */   {DS_OFF,   0,   5, DT_SWITCH,   Z_TOP_FLOOR, "Lampe Alice"},
  /* 01 */   {DS_OFF,   0,   6, DT_SWITCH,   Z_TOP_FLOOR, "Lampe Michel"},
  /* 02 */   {DS_NONE,  0,   5, DT_GROUP,    Z_TOP_FLOOR, "Lampes de chevet"},
  /* 03 */   {DS_NONE,  0,   7, DT_SCENE,    Z_TOP_FLOOR, "Dodo Alice"},
  /* 04 */   {DS_NONE,  0,   8, DT_SCENE,    Z_TOP_FLOOR, "Dodo Michel"},
  /* 05 */   {DS_OFF,   0,  85, DT_SWITCH,   Z_TOP_FLOOR, "Télé ami"},

  /* 06 */   {DS_OFF,   0,   1, DT_SWITCH,   Z_GROUND_FLOOR, "Lampe sur pied"},
  /* 07 */   {DS_OFF,   0,   4, DT_SWITCH,   Z_GROUND_FLOOR, "Lampe sur table"},
  /* 08 */   {DS_OFF,   0,   3, DT_SWITCH,   Z_GROUND_FLOOR, "Bibliothèques"},
  /* 09 */   {DS_OFF,   0,  89, DT_DIMMER,   Z_GROUND_FLOOR, "Salle à manger"},
  /* 10 */   {DS_OFF,   0,  90, DT_DIMMER,   Z_GROUND_FLOOR, "Cuisine"},
  /* 11 */   {DS_OFF,   0, 113, DT_DIMMER,   Z_GROUND_FLOOR, "Entrée"},
  /* 12 */   {DS_OFF,   0, 140, DT_SWITCH,   Z_GROUND_FLOOR, "Balcons"},

  /* 13 */   {DS_OFF,   0,   8, DT_SWITCH,   Z_GARAGE, "Garage extérieur"},
  /* 14 */   {DS_OFF,   0,   7, DT_SWITCH,   Z_GARAGE, "Garage intérieur"},
  /* 15 */   {DS_NO,    0,  37, DT_SELECTOR, Z_GARAGE, "Fermeture auto."},
  /* 16 */   {DS_OPEN,  0,  29, DT_CONTACT,  Z_GARAGE, "Porte"},
  /* 17 */   {DS_NONE,  0,  28, DT_PUSH_OFF, Z_GARAGE, "Fermer porte"},
    
  /* 18 */   {DS_OFF,   0, 138, DT_SWITCH,   Z_BASEMENT, "Marches sous-sol"},
  /* 19 */   {DS_OFF,   0,  72, DT_SWITCH,   Z_BASEMENT, "Lampe sofa"},
  /* 20 */   {DS_OFF,   0,  52, DT_SWITCH,   Z_BASEMENT, "Lampes télé"},
  /* 21 */   {DS_OFF,   0,  87, DT_SWITCH,   Z_BASEMENT, "Bureau"},
  /* 22 */   {DS_OFF,   0, 173, DT_SWITCH,   Z_BASEMENT, "Torchère"},
  /* 23 */   {DS_NONE,  0,   6, DT_GROUP,    Z_BASEMENT, "Sous-sol"},

  /* 24 */   {DS_DEFAULT,  0,   159, DT_SELECTOR, Z_HOUSE, "Calendrier"}
};

const uint16_t deviceCount = sizeof(devices)/sizeof(device_t);

int findDevice(devtype_t type, uint32_t idx) {
  for (int n = 0; n < deviceCount; n++) {
    if (devices[n].type == type && devices[n].idx == idx) 
      return n;
  }    
  return -1;
}

#ifdef BALLISTIC_ROTATION
int nextZone(int cdev) {
  zone_t currentZone = devices[cdev].zone;
  while (devices[cdev].zone == currentZone) {
    cdev++;
    if (cdev >= deviceCount)
      return 0;
  }
  return cdev;
}     

int prevZone(int cdev) {
  zone_t currentZone = devices[cdev].zone;
  while (devices[cdev].zone == currentZone) {
    cdev--;
    if (cdev < 0)
      return deviceCount-1;
  }
  return cdev;
}     
#endif

 /*
// dump devices to serial
void dumpDevices(void) {
  for (int i=0; i<deviceCount; i++) {
      Serial.println();
      Serial.printf("Zone: %s (%d)\n", zones[devices[i].zone], devices[i].zone);
      Serial.printf("Name: %s\n", devices[i].name);
      Serial.printf("Type: %s (%d)\n", devicetypes[devices[i].type], devices[i].type);
      Serial.printf("idx: %d\n", devices[i].idx); 
      Serial.printf("Status: %s (%d\n", (devicesstatus[devices[i].status], evices[i].status);
  } 
}  
*/ 

// Selectors. Currently there are two selectors in the table
selector_t selectors[] {
  {15, DS_NO, 2},      //Fermeture automatique du garage
  {24, DS_DEFAULT, 3}  //Calendrier
};

const uint16_t selectorCount = sizeof(selectors)/sizeof(selector_t);

int findSelector(int index) {
  for (int i=0; i<selectorCount; i++) {
    if (selectors[i].index == index) return i;
  }
  return -1;
}


// Groups. Currently there are two groups in the table
group_t groups[] {
    {2, 2, {0, 1}},       // Lampes de chevet {Lampe Michel, Lampe Alice}
    {23, 3, {19, 20, 22}} // Sous-sol, {Lampe sofa, Lampes télé, Torchère}
};

const uint16_t groupCount = sizeof(groups)/sizeof(group_t);

// Alerts. Currently two alerts are set up
alert_t alerts[] {
  {16, DS_OPEN},  // garage door : alert when open (DS_OPEN)
  {15, 0}         // automatic garage door closing : alert when turned off (DS_NO)
                  //   but this is a selector so status will be (DS_NO | DS_YES) - DS_NO
};

const uint16_t alertCount = sizeof(groups)/sizeof(group_t);

int currentAlert = -1;

int nextAlert(void) {
  //sendToLogPf(LOG_DEBUG, PSTR("nextAlert, currentAlert %d"), currentAlert);
  int next = currentAlert + 1;  
  for (int i=0; i < alertCount; i++) {
    if (next >= alertCount) next = 0;
    //sendToLogPf(LOG_DEBUG, PSTR("Checking alert %d, index %d, status %d, condition %d"), next, alerts[next].index, devices[alerts[next].index].status, alerts[next].condition);

    if ( devices[alerts[next].index].status == alerts[next].condition ) {
      if (currentAlert != next) {
        currentAlert = next;
        sendToLogPf(LOG_DEBUG, PSTR("Setting alert %d"), next);
      }  
      return currentAlert;
    }  
    next++;
  }
  // fallthrough, when no alerts
  if (currentAlert >= 0) {
    sendToLogP(LOG_DEBUG, PSTR("Stopping alert"));
    currentAlert = -1;
  }  
  return currentAlert;
}  

  
