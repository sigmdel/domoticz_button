/* Domoticz Button 

   An very small Domoticz controller running on an ESP8266 dev board with
   a small OLED display and a rotary encoder with a push button switch.

   Based on the Kitchen Button by David Conran (crankyoldgit)
   @ https://github.com/crankyoldgit/Kitchen-Button
  
   MQTT has to be enabled in Domoticz. See the MQTT wiki page 
   @ https://www.domoticz.com/wiki/MQTT

   TODO - 2020-12-20

    FIX 
      DT_CONTACT, DT_SELECTOR 
        - should be last in device types to avoid test in sending command
          and better handle toggling device

    ADD ?
      temps/humidity not shown
      garbage not shown
      tides not shown
*/                                                                

#include <Arduino.h>            // framework for platformIO

#include <ESP8266WiFi.h>        // WiFi support for ESP8266
#include <WiFiManager.h>        // WiFi configuration utility
#include <ESP8266HTTPClient.h>  // HTTP protocol support for ESP8266
#include <PubSubClient.h>       // MQTT client for Arduino framework
#include <ArduinoJson.h>        // JSON library

#include "SSD1306Wire.h"        // hardware driver for SSD1306 OLED display
#include "roboto14.h"           // OLED display font
#include "mdRotaryEncoder.h"    // hardware driver for rotary encoder
#include "mdPushButton.h"       // hardware driver for push button

#include "logging.h"             // logging to UART and syslog server
#include "config.h"              // application configuration
#include "sota.h"                // OTA firmware update
#include "lang.h"                // i8n

#include "devices.h"             // definitions of Domoticz devices, groups and scenes 


#ifndef SERIAL_BAUD
  #define SERIAL_BAUD 115200
#endif  

// Experimental "ballistic" encoder rotation
#ifndef BALLISTIC_ROTATION
  //#define BALLISTIC_ROTATION
#endif

WiFiClient mqttClient;
PubSubClient mqtt_client(mqttClient);


/* * * Domoticz button mode * * */

enum buttonMode_t { 
  BM_STATUS,       // showing/editing device On/Off status
                        // rotating the encoder moves to adjacent the device, 
                        // clicking once toogles the current device on/off
                        // clicking twice changes to BM_DIM_LEVEL if the current device is a dimmer or BM_SELECTOR if the current device is a selector
  BM_DIM_LEVEL,    // showing/editing dimmer level 
                        // rotating the encoder increases/descrease the dim level
                        // clicking once sets the dim level and change to BM_STATUS
                        // clicking twice changes to BM_STATUS, the dim level remains at initial value
  BM_SELECTOR,     // showing/editing selector choice 
                        // rotating the encoder shows previous/next selector choice
                        // clicking once sets the selector's choice changes to BM_STATUS
                        // clicking twice changes to BM_STATUS, the selector choice remains at initial value                         
  BM_BLANKED,      // showing nothing
                        // this state is entered when inactivity goes on for more than the config.displayTimeout
                        // rotating the encoder returns to BM_STATUS
                        // clicking the switch returns to BM_STATUS
                        // alerts can be flashed when in this mode
  BM_CONFIGURATION                      
} buttonMode = BM_CONFIGURATION;   // to ensure setting mode to BM_STATUS in setup()

enum configOptions_t {
  CO_FIRMWARE_UPDATE,
  CO_CONFIG_UPDATE,
  CO_DEFAULT_CONFIG,
  CO_CLEAR_WIFI,
  CO_SHOW_INFO,
  CO_RESTART  
};
#define configOptionsCount 6

bool displayNeedsUpdating = true;  // set to true to have the display updated in next loop cycle
bool displayVisible = true;        // is the display currently visible
unsigned long timeLastActive;      // used to decide when to turn the display off
int cdev = 0;                      // index of current device devices array
int8_t dimLevel = 0;               // temporary dim level 0-10 when editing dimmer
int8_t selChoice = 0;              // temporary selection choice when editing selector
int8_t configChoice = 0;           // temporary choice in the configuration mode

/************************/
/* * * OLED display * * */
/************************/

#define TOP_ROW 0 
#define MIDDLE_ROW 23
#define BOTTOM_ROW 46

#define SDA  D2  // gpio4
#define SCL  D1  // gpio5

SSD1306Wire display(0x3c, SDA, SCL, GEOMETRY_128_64);

void Show(char* top, char* middle, char* bottom, uint16_t waitTime = 0, bool alert = false) {
  display.displayOn();
  display.clear();
  display.drawString(64, TOP_ROW, top);
  display.drawString(64, MIDDLE_ROW, middle);
  if (alert) {
     display.fillRect(0, BOTTOM_ROW, display.width(), display.height() - BOTTOM_ROW+1);
     display.setColor(BLACK);
  }  
  display.drawString(64, BOTTOM_ROW, bottom);
  if (alert)
    display.setColor(WHITE);
  display.display();
  delay(waitTime);
}

void displayDevice(uint16_t index, bool alert=false) {
  // build bottom row
  char llbuf[32];  
  if (buttonMode == BM_DIM_LEVEL) {
      sprintf(llbuf, SC_BM_DIM_LEVEL, dimLevel * 10);
  } else if (buttonMode == BM_SELECTOR) {
      sprintf(llbuf, SC_BM_SELECTOR, devicestatus[ selChoice + selectors[devices[index].xstatus].status0 ]);
  } else {  // (buttonmode == BM_DEVICES)
    if (devices[index].type == DT_DIMMER)
      sprintf(llbuf, SC_BM_DEVICE_DIMMER, devicestatus[devices[index].status], devices[index].xstatus * 10);
    else if (devices[index].type == DT_SELECTOR) {
      sendToLogPf(LOG_DEBUG, PSTR("Selector %s, status %d, xstatus %d, status0 %d, statusCount %d"), 
        devices[index].name, devices[index].status, devices[index].xstatus, selectors[devices[index].xstatus].status0, selectors[devices[index].xstatus].statusCount);
      sprintf(llbuf, SC_BM_DEVICE_SELECTOR, devicestatus[ devices[index].status + selectors[devices[index].xstatus].status0 ]);
    } else {
      sprintf(llbuf, SC_BM_DEVICE_OTHER, devicestatus[devices[index].status]);  
    }  
  } 
  Show( (char*) zones[devices[index].zone], (char *) devices[index].name, llbuf, 0, alert);
  sendToLogPf(LOG_DEBUG, PSTR("Updated display for device %s.%s, alert %s, edit mode %s"), 
    zones[devices[index].zone], devices[index].name, (alert) ? "yes" : "no", (buttonMode == BM_STATUS) ? "BM_DEVICES" : "BM_DIMMER");
}

void displayConfiguration(void) {
  switch (configChoice) {
    case CO_FIRMWARE_UPDATE: Show( (char*) SC_CO_CONFIGURATION, (char*) SC_CO_FIRMWARE_UPDATE1, (char*) SC_CO_FIRMWARE_UPDATE2); break;
    case CO_CONFIG_UPDATE: Show( (char*) SC_CO_CONFIGURATION,  (char*) SC_CO_CONFIG_UPDATE1,  (char*) SC_CO_CONFIG_UPDATE2); break;
    case CO_DEFAULT_CONFIG: Show( (char*) SC_CO_CONFIGURATION,  (char*) SC_CO_DEFAULT_CONFIG1,  (char*) SC_CO_DEFAULT_CONFIG2); break;
    case CO_CLEAR_WIFI: Show( (char*) SC_CO_CONFIGURATION,  (char*) SC_CO_CLEAR_WIFI1,  (char*) SC_CO_CLEAR_WIFI2); break;
    case CO_SHOW_INFO: Show( (char*) SC_CO_CONFIGURATION,  (char*) SC_CO_SHOW_INFO1,  (char*) SC_CO_SHOW_INFO2); break;
    case CO_RESTART: Show( (char*) SC_CO_CONFIGURATION,  (char*) SC_CO_RESTART1,  (char*) SC_CO_RESTART2); break;
  }  
}

void doUpdateDisplay(void) {
  if (buttonMode == BM_CONFIGURATION)
    displayConfiguration();
  else  
    displayDevice(cdev);
  displayVisible = true;
  displayNeedsUpdating = false;
  timeLastActive = millis();
}

void displayInfo(void) {
  Show(config.hostname, (char*) SC_FIRMWARE_VERSION, (char*) String(VERSION).c_str(), config.infoTime);
  Show((char*) SC_WIFI_CONNECTED0, (char*) WiFi.localIP().toString().c_str(), (char*) "", config.infoTime);
  if (mqtt_client.connected()) 
    Show( (char*) SC_MQTT_CONNECTED0, (char*) SC_MQTT_CONNECTED1, (char*) config.mqttHost, config.infoTime);
  else 
    Show( (char*) SC_MQTT_NOT_CONNECTED0, (char*) SC_MQTT_NOT_CONNECTED1, (char*) config.mqttHost, config.infoTime);
}

/***********************/
/* * * Wi-Fi setup * * */
/***********************/

void doRestart(bool resetCredentials = false) {  
  if (resetCredentials) {
    WiFi.disconnect(true); // forget Wi-Fi credentials, will create Access point on reboot
  }  
  sendToLogPf(LOG_INFO, PSTR("Restarting %s"), APP_NAME);
  display.clear();
  display.drawString(64, MIDDLE_ROW, SC_RESTARTING);
  display.display();
  delay(config.infoTime);  // Enough time for messages to be sent.
  ESP.restart();
  while (1) ; //ensure this functino does not return.
}

//gets called when WiFiManager enters configuration mode
void WiFiManagerCallback(WiFiManager *myWiFiManager) {
  sendToLogPf(LOG_INFO, PSTR("Started access point with SSID: %s"),  myWiFiManager->getConfigPortalSSID().c_str());
  Show( (char*) SC_ACCESS_POINT, (char*) myWiFiManager->getConfigPortalSSID().c_str(), (char*) WiFi.softAPIP().toString().c_str());
}

void setup_wifi(void) {
  delay(10);
  //Local WiFiManager, once its business is done, there is no need to keep it around
  WiFiManager wm;
  //wm.resetSettings();
  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wm.setAPCallback(WiFiManagerCallback);
  // We start by connecting to a WiFi network
  wm.setTimeout(300);  // Time out after 5 mins.
  if (!wm.autoConnect())
    // Reboot. A.k.a. "Have you tried turning it Off and On again?"
    doRestart();
}


/****************/
/* * * MQTT * * */
/****************/

// mqtt payload: {"command":"switchlight", "idx":IDX, "switchcmd":"VAL"} where IDX = integer, VAL =  On or Off
// note 1. use for switches, push on, push off, dimmer (on/off only) etc.
const char switchcmd[] = "{\"command\":\"switchlight\", \"idx\":%d, \"switchcmd\":\"%s\"}";

// mqtt payload: {"command":"switchlight", "idx":IDX, "switchcmd":"Set Level", "level":VAL} where IDX = integer, VAL =  integer (0..100)
// note 1. VAL = 0 turns dimmer off without changing dim level 
// note 2. VAL > 0 sets dim level and turns light on
// note 3. same command for Dimmer and Selector, see https://www.domoticz.com/wiki/Domoticz_API/JSON_URL%27s#Selector_Switch
const char dimmercmd[] = "{\"command\":\"switchlight\", \"idx\":%d, \"switchcmd\":\"Set Level\", \"level\":%d}";

// mqtt payload: {"command":"switchscene", "idx":IDX, "switchcmd":"VAL"} where IDX = integer, VAL =  On or Off
// note 1. only ON possible for scenes
// note 2. ON or OFF possible for groups
const char  scenecmd[] = "{\"command\":\"switchscene\", \"idx\":%d, \"switchcmd\":\"%s\"}";

void send_domoticz_cmd(int dev, int32_t value, bool isLevel = false) {
  char buffer[MSG_SZ];    

  // Apparently sending { "command" : "switchlight", "idx" : 28, "switchcmd" : "Off" } to domoticz/in
  // does not cause Domoticz to send a domoticz/out MQTT message. Need to investigate this further.
  // In the mean time let's send the /out message directly to the IoT device
  if (devices[dev].type == DT_PUSH_OFF && devices[dev].idx == 28) {
    snprintf(buffer, MSG_SZ, "{ \"idx\" : %d, \"nvalue\" : 0 }", devices[dev].idx);
    mqtt_client.publish(DOMO_PUB_TOPIC, buffer); 
    sendToLogPf(LOG_INFO, PSTR("MQTT: publish [%s] %s"), DOMO_PUB_TOPIC, buffer);
    return;
  }  
  if ( devices[dev].type == DT_SWITCH || (devices[dev].type == DT_DIMMER && !isLevel) ) 
    snprintf(buffer, MSG_SZ, switchcmd, devices[dev].idx, (value) ? "On" : "Off");
  else if (devices[dev].type == DT_DIMMER || devices[dev].type == DT_SELECTOR)
    snprintf(buffer, MSG_SZ, dimmercmd, devices[dev].idx, value);
  else if (devices[dev].type == DT_SCENE && value) 
    snprintf(buffer, MSG_SZ, scenecmd, devices[dev].idx, "On");
  else if (devices[dev].type == DT_GROUP) 
    snprintf(buffer, MSG_SZ, scenecmd, devices[dev].idx, (value) ? "On" : "Off");
  else {
    sendToLogPf(LOG_ERR, PSTR("send_domoticz_cmd Not implement for type %s with value %d"), devicetypes[devices[dev].type], value);  
    return;
  }  
  mqtt_client.publish(DOMO_SUB_TOPIC, buffer);
  sendToLogPf(LOG_INFO, PSTR("MQTT: publish [%s] %s"), DOMO_SUB_TOPIC, buffer);
}    

void receivingMQTT(char const *topic, String const payload) {
  //sendToLogPf(LOG_DEBUG, PSTR("MQTT rx %s"), payload.c_str()); //payload.substring(0, 80).c_str());
  DynamicJsonDocument doc(config.mqttBufferSize);
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    sendToLogPf(LOG_ERR, PSTR("deserializeJson() failed : %s with message %s"), err.c_str(), payload.substring(0, 80).c_str());
    return;
  }
 
  int status;
  int xstatus = 0;
  devtype_t devType;
  
  int idx = doc["idx"];  // default 0 - not valid
  if (!idx) {
    sendToLogP(LOG_ERR, PSTR("idx not found in MQTT message"));
    return;
  }

  String sSwitchType = doc["switchType"];

  if (sSwitchType.isEmpty()) 
    sSwitchType = doc["Type"].as<String>();

  if (sSwitchType.isEmpty()) {
    sendToLogP(LOG_DEBUG, PSTR("Unknown device type in MQTT message"));
    return;
  }
  
  ////  if (sSwitchType.equals("On/Off"))
  if (sSwitchType == "On/Off")
    devType = DT_SWITCH;
  else if (sSwitchType == "Dimmer")
    devType = DT_DIMMER;
  else if (sSwitchType == "Contact")
    devType = DT_CONTACT;
  else if (sSwitchType == "Selector")
    devType = DT_SELECTOR;  
  else if (sSwitchType = "Group")  
    devType = DT_GROUP;
  else {
    sendToLogPf(LOG_DEBUG, PSTR("MQTT message %s ignored"), payload.substring(0, 80).c_str());
    return;
  }
  
  int i = findDevice(devType, idx);

  if (i < 0) {
    int p = 128;
    int n = payload.indexOf(String("\"name\" :"));
    if (n < 0) n = payload.indexOf(String("\"Name\" :"));
    if (n > 0) {
      p = payload.indexOf(':', n);
      n = p+1;
      p = payload.indexOf(',', n);
    } else
      n = 0;
    sendToLogPf(LOG_DEBUG, PSTR("Device named %s not handled"), payload.substring(n, p).c_str());
    return;
  }

  // get the device status and extra status if needed
  if (devType < DT_GROUP) {
    status = doc["nvalue"];
    if (devType == DT_DIMMER) {
      xstatus = doc["Level"];
    } else if (devType == DT_SELECTOR) {
      // status means nothing, replace with svalue1 
      status = doc["svalue1"].as<int>()/10;
      xstatus = findSelector(i);
    }  
  } else if (devType == DT_GROUP) {
     String sStatus = doc["Status"];
     if (sStatus.equals("On"))
       status = DS_ON;
     else if (sStatus.equals("Mixed"))
       status = DS_MIXED;
     else  
       status = DS_OFF;
  }  

  switch (devType) {
    case DT_SWITCH:   devices[i].status = (devstatus_t) (DS_OFF + status); break;
    case DT_DIMMER:   devices[i].status = (devstatus_t) (DS_OFF + status); 
                      devices[i].xstatus = xstatus / 10; break;
    case DT_CONTACT:  devices[i].status = (devstatus_t) (DS_CLOSED + status); break;
    case DT_SELECTOR: devices[i].status = (devstatus_t) status;
                      devices[i].xstatus = xstatus; break;
    case DT_GROUP:    devices[i].status = (devstatus_t) status; break;
    default: /* DT_SCENE, DT_PUSH_OFF: nothing to do */ break;
  }

  if (i == cdev && displayVisible) 
    displayNeedsUpdating = true;

  if (devType == DT_DIMMER || devType == DT_SELECTOR) 
    sendToLogPf(LOG_DEBUG, PSTR("Set %s status to %d, xstatus to %d"), devices[i].name, devices[i].status, devices[i].xstatus); 
  else   
    sendToLogPf(LOG_DEBUG, PSTR("Set %s status to %d"), devices[i].name, devices[i].status); 
}

// Callback function, when we receive an MQTT value on the topics
// subscribed this function is called
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  byte* payload_copy = reinterpret_cast<byte*>(malloc(length + 1));
  if (payload_copy == NULL) {
    sendToLogP(LOG_ERR, PSTR("Can't allocate memory for MQTT payload. Message ignored"));
    return;
  }
  // Copy the payload to the new buffer
  memcpy(payload_copy, payload, length);
  // Conversion to a printable string
  payload_copy[length] = '\0';

  // launch the function to treat received data
  receivingMQTT(topic, (char *)payload_copy);

  // Free the memory
  free(payload_copy);
}

const char infocmd[] = "{\"command\":\"get%sinfo\", \"idx\":%d}";

void mqttSubscribe(void) {
  char buffer[MSG_SZ];  
  mqtt_client.subscribe(DOMO_PUB_TOPIC);
  // update the status of all devices
  for (int i=0; i < deviceCount; i++) {
    if (devices[i].type <= DT_GROUP) {
      snprintf(buffer, MSG_SZ, infocmd, (devices[i].type == DT_GROUP) ? "scene" : "device", devices[i].idx);
      mqtt_client.publish(DOMO_SUB_TOPIC, buffer);
    }   
  }  
}

void mqttReconnect(void) {
  sendToLogP(LOG_DEBUG, PSTR("Reconnecting to MQTT broker"));  
  if (mqtt_client.connect(config.hostname)) {
    sendToLogPf(LOG_INFO, PSTR("Reconnected to MQTT broker %s as %s"), config.mqttHost, config.hostname);
    mqttSubscribe();
    Show( (char*) SC_MQTT_CONNECTED0, (char*) SC_MQTT_CONNECTED1, (char*) SC_MQTT_CONNECTED2, config.mqttUpdateTime);
  } else {
    sendToLogP(LOG_ERR, PSTR("Could not connect to MQTT broker"));  
    Show( (char*) SC_MQTT_NOT_CONNECTED0, (char*) SC_MQTT_NOT_CONNECTED1, (char*) SC_MQTT_NOT_CONNECTED2, config.infoTime);
  }
}


/*******************************/
/* * * Update group status * * */
/*******************************/

unsigned long updateGroupsTime = 0;

void updateGroupStatus(void) {
  for (int i=0; i < groupCount; i++) {
    devstatus_t stat = devices[groups[i].members[0]].status;
    for (int k=1; k < groups[i].count; k++) {
      if (devices[groups[i].members[k]].status != stat) {
        stat = DS_MIXED;
        break;
      }
    }
    if (devices[groups[i].index].status != stat) {
      (devices[groups[i].index].status = stat);
      if (groups[i].index == cdev)
        displayNeedsUpdating = true; 
      sendToLogPf(LOG_DEBUG, PSTR("Updated status of group %s to %s"), devices[groups[i].index].name, devicestatus[devices[groups[i].index].status]);
    }  
  }
}


/*************************/
/* * * Update alerts * * */
/*************************/

bool alertVisible = false;
unsigned long alertTime;

void doUpdateAlertDisplay(void) {
  if (nextAlert() < 0) {
    alertVisible = false;    
  }  else {
    displayDevice(alerts[currentAlert].index, true);
    alertVisible = true;
  }
}


/*******************************************/
/* * * Rotary encoder with push button * * */
/*******************************************/

#define pinClk D5 // gpio14
#define pinDt  D6 // gpio12
#define pinSw  D7 // gpio13

mdRotary rotary = mdRotary(pinClk, pinDt);
mdPushButton pushButton = mdPushButton(pinSw);  // mdPushButton(pinSw, LOW, true)

// Set Button mode 

const char* buttonModes[] = {
  "STATUS",
  "DIM_LEVEL",
  "SELECTOR",
  "BLANKED",
  "CONFIGURATION",
  "UNKNOWN"
};

void setButtonMode(buttonMode_t mode) {
  if (mode > BM_CONFIGURATION) 
    mode = (buttonMode_t) (BM_CONFIGURATION + 1);
  sendToLogPf(LOG_DEBUG, PSTR("Set buttonmode, currently BM_%s, to BM_%s"), buttonModes[buttonMode], buttonModes[mode]);  
  if (buttonMode != mode) {
    if (buttonMode == BM_BLANKED) {
      if (config.defaultDevice < deviceCount)
        cdev = config.defaultDevice;  
      display.displayOn();
    }
    switch (mode) {
      case BM_STATUS:
        rotary.setLimits(deviceCount-1);
        rotary.setPosition(cdev);
        break;
      case BM_DIM_LEVEL:
        dimLevel = devices[cdev].xstatus;
        rotary.setLimits(10);   // dimLevel 0 - 10
        rotary.setPosition(dimLevel);
        break;
      case BM_SELECTOR:
        selChoice = devices[cdev].status;
        rotary.setLimits(selectors[devices[cdev].xstatus].statusCount-1);
        rotary.setPosition(selChoice);
        break;
      case BM_BLANKED:
        displayVisible = false;
        alertTime = millis();
        alertVisible = false;
        display.displayOff();
        break;
      case BM_CONFIGURATION:
        configChoice = 0;
        rotary.setLimits(configOptionsCount-1);   
        rotary.setPosition(0);
        break;
      default: 
        sendToLogPf(LOG_ERR, PSTR("invalid buttonmode %d"), mode);  
        return;
        break;
    }      
    buttonMode = mode;
    displayNeedsUpdating = (buttonMode != BM_BLANKED);
  }
}

void toggleDevice(int dev) {
  if (devices[dev].type <= DT_GROUP && devices[dev].type != DT_SELECTOR && devices[dev].type != DT_CONTACT) {
    send_domoticz_cmd(dev, (devices[dev].status ==  DS_OFF) ? 1 : 0); // Turn device Off if it is on or mixed, on if it is off
    // Domoticz should send an mqtt message once it performs the task and on receiving it
    // the display will be updated, but groups status will not be updated
  } else if (devices[dev].type == DT_PUSH_OFF) {
    send_domoticz_cmd(dev, 0);  // push off buttons can only send off messages
  } else if (devices[dev].type == DT_SCENE) {
    send_domoticz_cmd(dev, 1);  // scenes can only be trigerred i.e. turned on
  } else { 
    sendToLogPf(LOG_ERR, PSTR("Cannot toggle %s a contact switch"), devices[dev].name);
    return;
  }  
  sendToLogPf(LOG_DEBUG, PSTR("Device %s status changed to %s"), devices[dev].name, devicestatus[devices[dev].status]);
}

void OnButtonClicked(int n) {
  sendToLogPf(LOG_DEBUG, PSTR("Button clicked %d times, buttonmode %s (%d), device %s (%d)"), n, buttonModes[buttonMode], buttonMode, devices[cdev].name, cdev);  

  if (n < 0) {
    setButtonMode(BM_CONFIGURATION);
    return;
  }

  if (buttonMode == BM_BLANKED) {
    if (n == 1 && config.defaultActive && config.defaultDevice < deviceCount) 
      toggleDevice(config.defaultDevice);
    setButtonMode(BM_STATUS); 
    return;  
  }

  if (buttonMode == BM_STATUS) {
    if (n == 1) {
      toggleDevice(cdev);  
    }
    if (n == 2) {
      if (devices[cdev].type == DT_DIMMER) {
        setButtonMode(BM_DIM_LEVEL);
      } else if (devices[cdev].type == DT_SELECTOR) {
        setButtonMode(BM_SELECTOR);
      }
    }
    return; 

  } else if (buttonMode == BM_CONFIGURATION) {
    if (n == 1)  {
      switch (configChoice) {
          case CO_FIRMWARE_UPDATE:    
            if (otaUpdate()) {
              clearEEPROM();         // will use default configuration on next boot, but keeps WiFi credentials
              sendToLogP(LOG_INFO, PSTR("Download firmware, clear EEPROM and restart")); 
              doRestart();
            } else {
              sendToLogP(LOG_DEBUG, "otaUpdate() failed");
              Show((char*) SC_FIRMWARE_FAIL0, (char*) SC_FIRMWARE_FAIL1, (char*) SC_FIRMWARE_FAIL2, config.infoTime);
            } 
            break;
          case CO_CONFIG_UPDATE: 
            if (updateConfig()) {
              sendToLogP(LOG_INFO, PSTR("Download configuration and restart")); 
              doRestart();
            } else {       
              Show((char*) "Échec en", (char *) "chargeant", (char*) "paramètres", config.infoTime); 
            }  
            break;
          case CO_DEFAULT_CONFIG: 
            sendToLogP(LOG_INFO, PSTR("Clear EEPROM, use default configuration and restart")); 
            clearEEPROM();  // otherwise the default will not be used after restart
            useDefaultConfig();
            doRestart();
            break;
          case CO_CLEAR_WIFI: 
            sendToLogP(LOG_INFO, PSTR("Clear WiFi credentials and restart")); 
            doRestart(true); 
            break;
          case CO_SHOW_INFO: 
            sendToLogP(LOG_INFO, PSTR("Show Info")); 
            displayInfo();
            break;
          case CO_RESTART: 
            sendToLogP(LOG_INFO, PSTR("Restart")); 
            doRestart(false); 
            break;
      }  
      // fall through if n != 1 or otaUpdate or updateConfig failed
    }

  } else { // buttonmode == BM_DIM_LEVEL || BM_SELECTOR    
    if (n == 1) {
      if (buttonMode == BM_DIM_LEVEL)
        send_domoticz_cmd(cdev, dimLevel * 10, true);  // domoticz dim level 0-100, dimLevel 0-10
      else if (buttonMode == BM_SELECTOR)
        send_domoticz_cmd(cdev, selChoice * 10, true);  // domoticz selectchoice 0, 10, 20 ....
        //; //send_domoticz_cmd(cdev )    
    }
    // fall through  
  }

  setButtonMode(BM_STATUS);
}

#ifdef BALLISTIC_ROTATION

// Esperiment in implementing a "balistic" rotation handler
// which moves to the next or previous zone when the
// button is quickly rotated

unsigned long lastRotationTime = 0;
int lastdir = 0;

void ButtonRotated(int32_t position) {
  int oldcdev = cdev;
  if (buttonMode == BM_STATUS) {
    if (millis() - lastRotationTime > 300) 
      cdev = position;
    else {
      if (position > cdev && lastdir > 0)  
        cdev = nextZone(cdev);
      else if (position < cdev && lastdir < 0)
        cdev = prevZone(cdev);
      else
        cdev = position;
    }
    lastdir = cdev - oldcdev;
    lastRotationTime = millis();
  } else if (buttonMode == BM_DIM_LEVEL)
    dimLevel = position;    
  else if (buttonMode == BM_SELECTOR)
    selChoice = position;  
  else if (buttonMode == BM_CONFIGURATION)
    configChoice = position;   
  else if (buttonMode == BM_BLANKED) {
    setButtonMode(BM_STATUS);
  }
  displayNeedsUpdating = true;
}

#else 

void ButtonRotated(int32_t position) {
  switch(buttonMode) {
    case BM_STATUS:        cdev = position; break;
    case BM_DIM_LEVEL:     dimLevel = position; break;
    case BM_SELECTOR:      selChoice = position; break;
    case BM_CONFIGURATION: configChoice = position; break;
    case BM_BLANKED:       setButtonMode(BM_STATUS); break;
  }
  displayNeedsUpdating = true;
}

#endif


/*****************/
/* * * setup * * */
/*****************/

void setup() {
  Serial.begin(SERIAL_BAUD);
  Serial.println("\n\nDomoticz Button"); // skip garbage
 
  int lc = loadConfig();
  sendToLogPf(LOG_INFO, PSTR("Starting %s"), APP_NAME);
  
  //if loglevel < LOG_INFO
  sendToLogP(LOG_INFO, (lc) ? PSTR("Using default configuration") :  PSTR("Loaded current configuration from flash memory"));

   // initialize display
  display.init();
  display.flipScreenVertically();
  display.setFont(Roboto_14);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  // show initial screen
  Show(config.hostname, (char*) SC_FIRMWARE_VERSION, (char*) String(VERSION).c_str(), config.infoTime);

  // intialize rotary encoder and push button
  rotary.onButtonRotated(ButtonRotated);
  pushButton.OnButtonClicked(OnButtonClicked);
  
  sendToLogP(LOG_DEBUG, PSTR("Starting Wifi radio"));
  setup_wifi();
  Show( (char*) SC_WIFI_CONNECTED0,  (char*) WiFi.localIP().toString().c_str(),  (char*) SC_WIFI_CONNECTED2, config.infoTime);
  
  if (config.autoFirmwareUpdate) {
    switch (checkForUpdates()) {
      case OTA_NEW_VERSION_LOADED:
        Show((char*) SC_FIRMWARE_LOADED0, (char*) SC_FIRMWARE_LOADED1, (char*) SC_FIRMWARE_LOADED2, config.infoTime);
        clearEEPROM();         // will use default configuration on next boot, but keeps WiFi credentials
        doRestart();
        break;
      case OTA_FAILED:
        Show((char*) SC_FIRMWARE_FAIL0, (char*) SC_FIRMWARE_FAIL1, (char*) SC_FIRMWARE_FAIL2, config.infoTime);
        break;
    case OTA_NO_NEW_VERSION: 
      /* continue */
      break;
    }   
  }

  // Finish setup of the mqtt clent object.
  sendToLogP(LOG_DEBUG, PSTR("MQTT setup"));
  if (!mqtt_client.setBufferSize(config.mqttBufferSize))
    sendToLogPf(LOG_ERR, PSTR("Could not allocated %d byte MQTT buffer"), config.mqttBufferSize);
  sendToLogPf(LOG_DEBUG, PSTR("Setting MQTT server: %s:%d"), config.mqttHost, config.mqttPort);   
  mqtt_client.setServer(config.mqttHost, config.mqttPort);
  mqtt_client.setCallback(mqttCallback);
  mqttReconnect();

  setButtonMode(BM_STATUS);

  sendToLogP(LOG_DEBUG, PSTR("Setup completed"));
}


/****************/
/* * * loop * * */
/****************/


//#define FAKE_OPEN_GARAGE_DOOR

#ifdef FAKE_OPEN_GARAGE_DOOR
unsigned long FAKEopenTime = millis();
#endif

// Timer to avoid trying to reconnect to the MQTT broker in a tight
// loop which will prevent processing button presses. Waiting
// 1 minute between attemps seems reasonable
#define MQTT_CONNECT_INTERVAL 60000   
unsigned long lastMqttConnectAttempt = 0;

void loop(void) {
#ifdef FAKE_OPEN_GARAGE_DOOR
  if (millis() - FAKEopenTime > 2*60*1000) {
    devices[18].status = (devices[18].status == DS_OPEN) ? DS_CLOSED : DS_OPEN;
    sendToLogPf(LOG_DEBUG, PSTR("Set garage door %s"), devicestatus[devices[18].status]));
    FAKEopenTime = millis();
  }  
#endif
  rotary.process();
  pushButton.status();

  if (millis() - updateGroupsTime > 10*1000) {
    updateGroupStatus();
    updateGroupsTime = millis();
  }  

  if (displayNeedsUpdating) 
    doUpdateDisplay();

  if (displayVisible && millis() - timeLastActive > config.displayTimeout) 
    setButtonMode(BM_BLANKED);

  if (!displayVisible && millis() - alertTime > config.alertTime) { // at regular intervals while every 3 seconds change something 
    alertTime = millis(); // restart timer
    if (alertVisible) { // turn off a visible alert
      alertVisible = false;
      display.displayOff();
      sendToLogP(LOG_DEBUG, PSTR("turning alert off"));
    } else {  // no alert shown, show one if possible)
      doUpdateAlertDisplay();
    }  
  }
  
  if (!mqtt_client.connected()) {
   if (millis() - lastMqttConnectAttempt > MQTT_CONNECT_INTERVAL) {
     lastMqttConnectAttempt = millis();
      mqttReconnect();
   }   
  } else {
    mqtt_client.loop();
  }  
}  
