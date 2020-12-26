#include <Arduino.h>
#include "devices.h"
#include "logging.h"


const char * zones[] = {"Top Floor", "Ground Floor", "Basement", "Garage", "House"};
// French version:
//const char * zones[] = {"Étage", "Rez-de-chaussée", "Sous-sol", "Garage", "Maison"};

const char * devicestatus[] = {"", "Off", "On", "Mixed", "No", "Yes", "Closed", "Open", "Default", "Weekend", "Holidays"};
// French version:
//const char * devicestatus[] = {"", "·", "¤", "·/¤", "Non", "Oui", "Fermée", "Ouverte", "Normal", "Absent", "Vacances"};

// only shown in logging messages
const char * devicetypes[] = {"switch", "dimmer", "contact", "selector", "group", "push off", "scene"};

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
                  //   but that corresponds to selector level 0
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

  
