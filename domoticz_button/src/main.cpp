/* Domoticz Button 

   An very small Domoticz controller running on an ESP8266 dev board with
   a small OLED display and a rotary encoder with a push button switch.

   Based on the Kitchen Button by David Conran (crankyoldgit)
   @ https://github.com/crankyoldgit/Kitchen-Button
  
   MQTT has to be enabled in Domoticz. See the MQTT wiki page 
   @ https://www.domoticz.com/wiki/MQTT
*/

#include <Arduino.h>            // framework for platformIO

#include <ESP8266WiFi.h>        // WiFi support for ESP8266
//#include <ESP8266mDNS.h>        // Multicast DNS (local host name to IP resolver)
//#include <WiFiClient.h>
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

WiFiClient mqttClient;
PubSubClient mqtt_client(mqttClient);


/* * * Domoticz button mode * * */

enum editmode_t { 
   EM_STATUS,       // showing/editing device On/Off status
                         // rotating the encoder moves to adjacent the device, 
                         // clicking once toogles the current device on/off
                         // clicking twice changes to EM_DIM_LEVEL if the current device is a dimmer or EM_SELECTOR if the current device is a selector
   EM_DIM_LEVEL,    // showing/editing dimmer level 
                         // rotating the encoder increases/descrease the dim level
                         // clicking once sets the dim level and change to EM_STATUS
                         // clicking twice changes to EM_STATUS, the dim level remains at initial value
   EM_SELECTOR,     // showing/editing selector choice 
                         // rotating the encoder shows previous/next selector choice
                         // clicking once sets the selector's choice changes to EM_STATUS
                         // clicking twice changes to EM_STATUS, the selector choice remains at initial value
   EM_NONE          // showing nothing
                         // this state is entered when inactivity goes on for more than the config.displayTimeout
                         // rotating the encoder returns to EM_STATUS
                         // clicking the switch returns to EM_STATUS
                         // alerts can be flashed when in this mode
} editmode = EM_STATUS;   


bool displayNeedsUpdating = true;  // set to true to have the display updated in next loop cycle
bool displayVisible = true;        // is the display currently visible
unsigned long timeLastActive;      // used to decide when to turn the display off
int cdev = 0;                      // index of current device devices array
int8_t dimLevel = 0;               // temporary dim level 0-10 when editing dimmer
int8_t selChoice = 0;              // temporary selection choice when editing selector


/************************/
/* * * OLED display * * */
/************************/

#define TOP_ROW 0 
#define MIDDLE_ROW 23
#define BOTTOM_ROW 46

#define SDA  D2  // gpio4
#define SCL  D1  // gpio5

SSD1306Wire display(0x3c, SDA, SCL, GEOMETRY_128_64);


void displayDevice(uint16_t index, bool alert=false) {
  char llbuf[32];  // for BOTTOM_ROW

  display.displayOn();
  display.clear();

  // display zone and device names no matter if setting device status or dimmer level
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(64, TOP_ROW, zones[devices[index].zone]);
  display.drawString(64, MIDDLE_ROW, devices[index].name);

  // build bottom row
  if (editmode == EM_DIM_LEVEL) {
      sprintf(llbuf, SC_EM_DIM_LEVEL, dimLevel * 10);
  } else if (editmode == EM_SELECTOR) {
      sprintf(llbuf, SC_EM_SELECTOR, devicestatus[ selChoice + selectors[devices[index].xstatus].status0 ]);
  } else {  // (editmode == EM_DEVICES)
    if (devices[index].type == DT_DIMMER)
      sprintf(llbuf, SC_EM_DEVICE_DIMMER, devicestatus[devices[index].status], devices[index].xstatus * 10);
    else if (devices[index].type == DT_SELECTOR) {
      sendToLogPf(LOG_DEBUG, PSTR("Selector %s, status %d, xstatus %d, status0 %d, statusCount %d"), 
        devices[index].name, devices[index].status, devices[index].xstatus, selectors[devices[index].xstatus].status0, selectors[devices[index].xstatus].statusCount);
      sprintf(llbuf, SC_EM_DEVICE_SELECTOR, devicestatus[ devices[index].status + selectors[devices[index].xstatus].status0 ]);
    } else {
      sprintf(llbuf, SC_EM_DEVICE_OTHER, devicestatus[devices[index].status]);  
    }  
  } 

  // display status
  if (alert) {
     display.fillRect(0, BOTTOM_ROW, display.width(), display.height() - BOTTOM_ROW+1);
     display.setColor(BLACK);
  }
  display.drawString(64, BOTTOM_ROW, llbuf);

  if (alert)
    display.setColor(WHITE);

  display.display(); // show it!  
  sendToLogPf(LOG_DEBUG, PSTR("Updated display for device %s.%s, alert %s, edit mode %s"), 
    zones[devices[index].zone], devices[index].name, (alert) ? "yes" : "no", (editmode == EM_STATUS) ? "EM_DEVICES" : "EM_DIMMER");
}

void doUpdateDisplay(void) {
  displayDevice(cdev);
  displayVisible = true;
  displayNeedsUpdating = false;
  timeLastActive = millis();
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
  Serial.println("Started access point");
  Serial.printf("SSID: %s\n", myWiFiManager->getConfigPortalSSID().c_str());
  Serial.printf("IP: %s\n", WiFi.softAPIP().toString().c_str());
 
  display.clear();
  display.drawString(64, TOP_ROW, SC_ACCESS_POINT);
  display.drawString(64, MIDDLE_ROW, myWiFiManager->getConfigPortalSSID());
  display.drawString(64, BOTTOM_ROW, WiFi.softAPIP().toString());
  display.display();
}

void setup_wifi(void) {
  delay(10);

  //Local WiFiManager, once its business is done, there is no need to keep it around
  WiFiManager wm;

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

  if (i == cdev) 
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
    display.clear();
    display.drawString(64, TOP_ROW,    SC_MQTT_CONNECTED0); // "Connected to" / "Connecté au"
    display.drawString(64, MIDDLE_ROW, SC_MQTT_CONNECTED1); // "MQTT broker"  / "serveur MQTT"
    display.drawString(64, BOTTOM_ROW, SC_MQTT_CONNECTED2); // "Updating..."  / "Mise à jour..."
    display.display();    
    mqttSubscribe();
    delay(config.mqttUpdateTime);
  } else {
    sendToLogP(LOG_ERR, PSTR("Could not connect to MQTT broker"));  
    display.clear();
    display.drawString(64, TOP_ROW,    SC_MQTT_NOT_CONNECTED0); // "Not connected"  / "Déconnecté du"
    display.drawString(64, MIDDLE_ROW, SC_MQTT_NOT_CONNECTED1); // "to MQTT broker" / "serveur MQTT"
    display.drawString(64, BOTTOM_ROW, SC_MQTT_NOT_CONNECTED2); // "..."            / "..."
    display.display();     
    delay(config.infoTime);
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


void OnButtonClicked(int n) {
  sendToLogPf(LOG_DEBUG, PSTR("Button clicked %d times, editmode %s (%d), device %s (%d)"), n, (editmode) ? "EM_DIMMER" : "EM_DEVICE", editmode, devices[cdev].name, cdev);  

  if (n < 0) {
    sendToLogP(LOG_INFO, PSTR("Button held down for long press"));
    doRestart(true);  // Reload configuration and then forget WiFi credentials and then reboot
  }

  if (n > 3) {
    sendToLogP(LOG_INFO, PSTR("Reload configuration"));
    if (updateConfig()) doRestart();
    // should display a message on display!!!
  }

  if (editmode == EM_STATUS) {
    if (n == 1) {
       if (devices[cdev].type <= DT_GROUP && devices[cdev].type != DT_SELECTOR) {
         sendToLogPf(LOG_DEBUG, PSTR("Device %s status = %s"), devices[cdev].name, devicestatus[devices[cdev].status]);
         send_domoticz_cmd(cdev, (devices[cdev].status ==  DS_OFF) ? 1 : 0); // Turn device Off if it is on or mixed, on if it is off
         // Domoticz should send an mqtt message once it performs the task and on receiving it
         // the display will be updated, but groups status will not be updated
       } else if (devices[cdev].type == DT_PUSH_OFF) {
          send_domoticz_cmd(cdev, 0);  // push off buttons can only send off messages
       } else if (devices[cdev].type == DT_SCENE) {
          send_domoticz_cmd(cdev, 1);  // scenes can only be trigerred i.e. turned on
       }   
    }
    if (n == 2) {
      if (devices[cdev].type == DT_DIMMER) {
        editmode = EM_DIM_LEVEL;
        dimLevel = devices[cdev].xstatus;
        rotary.setLimits(10);   // dimLevel 0 - 10
        rotary.setPosition(dimLevel);
        displayNeedsUpdating = true; // nothing to do with Domoticz, update the display manully
        sendToLogPf(LOG_DEBUG, PSTR("editmode set to %s"), "EM_DIMMER");  
      } else if (devices[cdev].type == DT_SELECTOR) {
        editmode = EM_SELECTOR;
        selChoice = devices[cdev].status;
        rotary.setLimits(selectors[devices[cdev].xstatus].statusCount-1);
        //rotary.setPosition(dimLevel);
        displayNeedsUpdating = true; // nothing to do with Domoticz, update the display manully
        sendToLogPf(LOG_DEBUG, PSTR("editmode set to %s"), "EM_SELECTION");  
      }
    }
    return;   
  } else if (editmode == EM_NONE) {
    editmode = EM_STATUS;
    displayNeedsUpdating = true;
  } else { // editmode == EM_DIM_LEVEL || EM_SELECTOR
    
    if (n == 1) {
      if (editmode == EM_DIM_LEVEL)
        send_domoticz_cmd(cdev, dimLevel * 10, true);  // domoticz dim level 0-100, dimLevel 0-10
      else if (editmode == EM_SELECTOR)
        send_domoticz_cmd(cdev, selChoice * 10, true);  // domoticz selectchoice 0, 10, 20 ....
        //; //send_domoticz_cmd(cdev )    
    }
        
    // if n > 2 don't change dim value or selector choice
    // return to editing devices no matter the number of button presses
    editmode = EM_STATUS;
    rotary.setLimits(deviceCount-1);
    rotary.setPosition(cdev);;
    sendToLogPf(LOG_DEBUG, PSTR("editmode set to %s"), "EM_DEVICES");  
    displayNeedsUpdating = true;
  }    
}

void ButtonRotated(int32_t position) {
  if (editmode == EM_STATUS)
    cdev = position;
  else if (editmode == EM_DIM_LEVEL)
    dimLevel = position;    
  else if (editmode == EM_SELECTOR)
    selChoice = position;  
  else if (editmode == EM_NONE)
    editmode = EM_STATUS;  
  displayNeedsUpdating = true;
}


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

 
  display.init();
  display.flipScreenVertically();
  display.setFont(Roboto_14);
  display.setTextAlignment(TEXT_ALIGN_CENTER);

  display.clear();
  display.drawString(64, TOP_ROW, config.hostname);
  display.drawString(64, MIDDLE_ROW, SC_FIRMWARE_VERSION); // "version" / "version"
  display.drawString(64, BOTTOM_ROW, String(VERSION));
  display.display();
  delay(config.infoTime);


  // init rotary encoder
  rotary.setLimits(deviceCount-1);
  rotary.setPosition(0); // start at 
  rotary.onButtonRotated(ButtonRotated);
  
  // init push button 
  pushButton.OnButtonClicked(OnButtonClicked);
 
  sendToLogP(LOG_DEBUG, PSTR("Starting Wifi radio"));
  setup_wifi();
 
  display.clear();
  display.drawString(64, TOP_ROW,    (SC_WIFI_CONNECTED0 == "%IP%") ? WiFi.localIP().toString() : SC_WIFI_CONNECTED0);
  display.drawString(64, MIDDLE_ROW, (SC_WIFI_CONNECTED1 == "%IP%") ? WiFi.localIP().toString() : SC_WIFI_CONNECTED1);
  display.drawString(64, BOTTOM_ROW, (SC_WIFI_CONNECTED2 == "%IP%") ? WiFi.localIP().toString() : SC_WIFI_CONNECTED2);
  display.display();
  delay(config.infoTime);

  switch (checkForUpdates()) {
    case OTA_NEW_VERSION_LOADED:
      display.clear();
      display.drawString(64, TOP_ROW,    SC_FIRMWARE_LOADED0);  // "New firmware" / "Nouveau"
      display.drawString(64, MIDDLE_ROW, SC_FIRMWARE_LOADED1);  // "loaded"       / "micrologiciel"  
      display.drawString(64, BOTTOM_ROW, SC_FIRMWARE_LOADED2);  // ""             / "téléchargé"
      display.display();
      delay(config.infoTime);
      clearEEPROM();         // will use default configuration on next boot, but keeps WiFi credentials
      doRestart();
      break;
    case OTA_FAILED:
      display.clear();
      display.drawString(64, TOP_ROW, SC_FIRMWARE_FAIL0);     //"Failed to load" / "Échec en"
      display.drawString(64, MIDDLE_ROW, SC_FIRMWARE_FAIL1);  //"new firmware"   / "chargeant le"
      display.drawString(64, BOTTOM_ROW, SC_FIRMWARE_FAIL2);  //""               / "micrologiciel"
      display.display();
      delay(config.infoTime);
      break;
   case OTA_NO_NEW_VERSION: 
     /* continue */
     break;
  }   

  // Finish setup of the mqtt clent object.
  sendToLogP(LOG_DEBUG, PSTR("MQTT setup"));
  if (!mqtt_client.setBufferSize(config.mqttBufferSize))
    sendToLogPf(LOG_ERR, PSTR("Could not allocated %d byte MQTT buffer"), config.mqttBufferSize);
  sendToLogPf(LOG_DEBUG, PSTR("Setting MQTT server: %s:%d"), config.mqttHost, config.mqttPort);   
  mqtt_client.setServer(config.mqttHost, config.mqttPort);
  mqtt_client.setCallback(mqttCallback);
  mqttReconnect();

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

  if (displayVisible && millis() - timeLastActive > config.displayTimeout) {
    displayVisible = false;
    display.displayOff();
    alertTime = millis();
    alertVisible = false;
    editmode = EM_NONE;
  }  

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
