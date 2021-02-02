#include <Arduino.h>
#include <cstdint>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include "config.h"
#include "logging.h"

#define MAX_EEPROM_SIZE 4096

config_t config;
uint32_t eepromConfigHash;

uint32_t getConfigHash() {
  uint32_t hash = 0;
  uint8_t *bytes = (uint8_t*)&config;

  for (uint16_t i = 0; i < sizeof(config_t) - sizeof(config.checksum); i++) hash += bytes[i]*(i+1);
  return hash;
}

/*
 * https://esp8266.github.io/Arduino/versions/2.0.0/doc/libraries.html
 * 
 * This is a bit different from standard EEPROM class. You need to call EEPROM.begin(size) 
 * before you start reading or writing, size being the number of bytes you want to use. 
 * 
 * Size can be anywhere between 4 and 4096 bytes.
 * 
 * EEPROM.write does not write to flash immediately, instead you must call EEPROM.commit() 
 * whenever you wish to save changes to flash. EEPROM.end() will also commit, and will 
 * release the RAM copy of EEPROM contents.
 * 
 * EEPROM library uses one sector of flash located just after the SPIFFS.
 * 
 * Looking at EEPROM.write code in https://github.com/esp8266/Arduino/blob/master/libraries/EEPROM/EEPROM.cpp
 * it seems clear that a lazy write algorithm is used in EEPROM.commit(), meaning a ram copy of 
 * the content of the EEPROM is used as a buffer and only if changes are made to that copy with 
 * EEPROM.write will there be a change.
 */

void useDefaultConfig(void) {
  memset(&config, 0x00, sizeof(config_t));
  config.magic = CONFIG_MAGIC;
  
  strlcpy(config.hostname, HOST_NAME, HOST_NAME_SZ);

  strlcpy(config.mqttHost, MQTT_HOST, URL_SZ);
  config.mqttPort = MQTT_PORT;
  strlcpy(config.mqttUser, MQTT_USER, SSID_SZ);
  strlcpy(config.mqttPswd, MQTT_PSWD, PSWD_SZ);
  config.mqttBufferSize = MQTT_BUFFER_SIZE;

  strlcpy(config.syslogHost, SYSLOG_HOST, URL_SZ);
  config.syslogPort = SYSLOG_PORT;

  strlcpy(config.otaHost, OTA_HOST, URL_SZ);
  config.otaPort = OTA_PORT;
  strlcpy(config.otaUrlBase, OTA_URL_BASE, HOST_NAME_SZ);
  config.autoFirmwareUpdate =  OTA_AUTO_FIRMWARE_UPDATE;
  
  config.defaultDevice = DEFAULT_DEVICE;
  config.defaultActive = DEFAULT_ACTIVE;
  
  config.displayTimeout = DISPLAY_TIMEOUT*1000;
  config.alertTime = ALERT_TIME*1000;
  config.infoTime = INFO_TIME*1000;
  config.mqttUpdateTime = MQTT_UPDATE_TIME*1000;
  config.suspendBuzzerTime = SUSPEND_BUZZER_TIME;

  config.logLevelUart = LOG_LEVEL_UART;
  config.logLevelSyslog = LOG_LEVEL_SYSLOG;
 
  config.checksum = getConfigHash();
  sendToLogPf(LOG_DEBUG, PSTR("Using default config, checksum = 0x%x08"), config.checksum);
} 


// save config to flash memory
//
void saveConfigToEEPROM(void) {
  config.checksum = getConfigHash();
  EEPROM.begin(sizeof(config_t));
  EEPROM.put(0, config);
  EEPROM.end();
  sendToLogPf(LOG_DEBUG, PSTR("Saved current configuration to flash memory, checksum 0x%x08)"), config.checksum);
}


// Copies configuration saved in EEPROM to config buffer and calculates
// the hash for the config buffer. If the EEPROM saved configuration is 
// an older version, if will be shorter than the current config size
// and garbage (ie. 0x00) will be at the end. 
//
uint32_t loadConfigFromEEPROM(void) {
  memset(&config, 0x00, sizeof(config_t));
  EEPROM.begin(sizeof(config_t));
  EEPROM.get(0, config);
  EEPROM.end();
  return getConfigHash();
}

// returns 0 if worked, 1 if loadec default config 
loadConfig_t loadConfig(void) {
  eepromConfigHash = loadConfigFromEEPROM();
  if (config.magic != CONFIG_MAGIC || eepromConfigHash != config.checksum ) {
    // if (config.magic != CONFIG_MAGIC)
    //  sendToLogP(LOG_DEBUG, PSTR("Config magic id wrong"));
    // if (eepromConfigHash != config.checksum) 
    //  sendToLogPf(LOG_DEBUG, "Saved config checksum: 0x%x08, calculated: 0x%x08", config.checksum, eepromConfigHash);
    useDefaultConfig();
    eepromConfigHash = config.checksum;
    return LC_FROM_DEFAULTS;
  }
  return LC_FROM_EEPROM;
}

void clearEEPROM(void) {
  EEPROM.begin(sizeof(uint16_t));
  for (uint16_t i = 0 ; i < sizeof(uint16_t) ; i++) {
    EEPROM.write(i, 0xFF);
  }
  EEPROM.end();  
  sendToLogP(LOG_DEBUG, PSTR("Cleared configuration in EEPROM")); 
}

//******************************************************************
// updateConfig
//******************************************************************

void paramNotFound(const char* value) {
  sendToLogPf(LOG_DEBUG, PSTR("Parameter \"%s\" not found in JSON file"), value);
}

bool obtainJsonStr(DynamicJsonDocument doc, const char* value, char* buf, int maxSize) {
  String sData = doc[value];
  if (!sData.isEmpty() && !sData.equalsIgnoreCase("null")) {
    strlcpy(buf, sData.c_str(), maxSize);
    sendToLogPf(LOG_INFO, PSTR("Updating config.%s to \"%s\""), value, buf);
    return true;
  } else {
    paramNotFound(value);
    return false;
  }      
}

bool obtainJsonInt(DynamicJsonDocument doc, char* value, uint32_t* buf) {
  String sData = doc[value];
  //Serial.printf("sData: %s\n", sData.c_str());
  if (!sData.isEmpty() && !sData.equalsIgnoreCase("null")) {
    *buf = (uint32_t) sData.toInt();
    sendToLogPf(LOG_INFO, PSTR("Updating config.%s to %d"), value, buf);
    return true;
  } else {
    paramNotFound(value);
    return false;
  }  
}

bool obtainJsonLevel(DynamicJsonDocument doc, char* value, uint8_t* buf) {
  char buffer[32];
  int numb = -1;
  if (obtainJsonStr(doc, value, (char*) &buffer, 32)) {
     String level = buffer;
     level.toUpperCase();
     level.replace("LOG_TO", "");
     if (level.equals("EMERG")) numb = LOG_EMERG;
     else if (level.equals("ALERT")) numb = LOG_ALERT;
     else if (level.equals("CRIT")) numb = LOG_CRIT;
     else if (level.equals("ERR")) numb = LOG_ERR;
     else if (level.equals("WARNING")) numb = LOG_WARNING;
     else if (level.equals("NOTICE")) numb = LOG_NOTICE;
     else if (level.equals("INFO")) numb = LOG_INFO;
     else if (level.equals("DEBUG")) numb = LOG_DEBUG;
     if (numb >= LOG_EMERG && numb <= LOG_DEBUG) {
       *buf = (uint8_t) numb;
       sendToLogPf(LOG_INFO, PSTR("Updating config.%s to LOG_%s"), value, level.c_str());
       return true;
     }
    sendToLogPf(LOG_ERR, PSTR("\"%s\" is not a valie log level (config.%s)"), buffer, value);
    return false;
  }
  paramNotFound(value);
  return false;
}

bool loadJsonConfig(String& filecontent) {
  WiFiClient wifiClient;
  HTTPClient httpClient;
  filecontent = "";
  bool result = false;

  String configFile = String(config.otaUrlBase) + config.hostname + ".config.json";
  String configURL = String("http://") + config.otaHost + ":" + config.otaPort;
  //Serial.printf("Request %s%s\n", configURL.c_str(), configFile.c_str());

  if (httpClient.begin(wifiClient, configURL + configFile)) {
    int httpCode = httpClient.GET();
    if (httpCode == HTTP_CODE_NOT_FOUND) {
      sendToLogPf(LOG_ERR, PSTR("File \"%s\" not found on %s"), configFile.c_str(), configURL.c_str());
    } else if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
      // file found at server
      filecontent = httpClient.getString();
      result = true;
    } else {  
      sendToLogPf(LOG_ERR, PSTR("Download failed. HTTP error %s (%d)"), httpClient.errorToString(httpCode).c_str(), httpCode);
    }
    httpClient.end();
  } else {
    sendToLogPf(LOG_ERR, PSTR("Unable to connect to %s"), configURL.c_str());
  }
  return result;
}

bool updateConfig(void) {
  uint32_t numb;
  String filecontent;

  if (!loadJsonConfig(filecontent)) return false;
 
  DynamicJsonDocument doc(3*filecontent.length());
  DeserializationError err = deserializeJson(doc, filecontent);
  if (err) {
    sendToLogPf(LOG_ERR, PSTR("Parsing JSON (%s...) failed: %s"), filecontent.substring(0, 80).c_str(), err.c_str());
    return false;
  }
 
  sendToLogP(LOG_INFO, PSTR("Updating configuration from JSON file"));
  
  obtainJsonStr(doc, (char*) "hostname", (char*) &config.hostname, URL_SZ);

  obtainJsonStr(doc, (char*) "mqttHost", (char*) &config.mqttHost, URL_SZ);
  if ( obtainJsonInt(doc, (char*) "mqttPort", &numb) ) config.mqttPort = numb;
  obtainJsonStr(doc, (char*) "mqttUser", (char*) &config.mqttUser, SSID_SZ);
  obtainJsonStr(doc, (char*) "mqttPswd", (char*) &config.mqttPswd, PSWD_SZ);
  if ( obtainJsonInt(doc, (char*) "mqttBufferSize", &numb) ) config.mqttBufferSize = numb;

  obtainJsonStr(doc, (char*) "syslogHost", (char*) &config.syslogHost, URL_SZ);
  if ( obtainJsonInt(doc, (char*) "syslogPort", &numb) ) config.syslogPort = numb;
  
  obtainJsonStr(doc, (char*) "otaHost", (char*) &config.otaHost, URL_SZ);
  if ( obtainJsonInt(doc, (char*) "otaPort", &numb) ) config.otaPort = numb;
  obtainJsonStr(doc, (char*) "otaUrlBase", (char*) &config.otaUrlBase, URL_SZ);
  if ( obtainJsonInt(doc, (char*) "autoFirmwareUpdate", &numb) ) config.autoFirmwareUpdate = numb;

  if ( obtainJsonInt(doc, (char*) "defaultDevice", &numb) ) config.defaultDevice = numb;
  if ( obtainJsonInt(doc, (char*) "defaultActive", &numb) ) config.defaultActive = numb;

  if (obtainJsonInt(doc, (char*) "displayTimeout", &numb)) config.displayTimeout = numb*1000;
  if (obtainJsonInt(doc, (char*) "alertTime", &numb)) config.alertTime = numb*1000;
  if (obtainJsonInt(doc, (char*) "infoTime", &numb)) config.infoTime = numb*1000;
  if (obtainJsonInt(doc, (char*) "mqttUpdateTime", &numb)) config.mqttUpdateTime = numb*1000;
  if (obtainJsonInt(doc, (char*) "suspendBuzzerTime", &numb)) config.suspendBuzzerTime = numb;

  if (obtainJsonLevel(doc, (char*) "logLevelUart", (uint8_t*) &numb)) config.logLevelUart = numb;
  if (obtainJsonLevel(doc, (char*) "logLevelSyslog", (uint8_t*) &numb)) config.logLevelSyslog = numb;    

  if (getConfigHash() != config.checksum) {
    config.checksum = getConfigHash();
    /// do some sanity verifications ?
    /// mqttHost == "" or mqttPort == "" or hostname == "" then reload configuration
    saveConfigToEEPROM(); 
  }
  return true;
}


/*******************************************************************/
#ifdef DEBUG_CONFIG

#define CFG_COLS 16

void clearConfig(config_t* cfg) {
  memset(cfg, 0x00, sizeof(config_t));
}

// should refactor this... but its not production code

void dumpConfig(config_t * cfg, char * msg) { 
  Serial.println(msg);
  Serial.printf("  magic: 0x%04X (%c%c)\n", cfg->magic, cfg->magic >> 8, cfg->magic & 0xFF);
  Serial.printf("  hostname: \"%s\"\n", cfg->hostname);
  Serial.printf("  mqtthost: \"%s\"\n", cfg->mqttHost);
  Serial.printf("  mqttPort: %d\n", cfg->mqttPort);
  Serial.printf("  mqttUser: \"%s\"\n", cfg->mqttUser);
  Serial.printf("  mqttPswd: \"%s\"\n", cfg->mqttPswd);
  Serial.printf("  mqttBufferSize: %d\n", cfg->mqttBufferSize);
  Serial.printf("  syslogHost: \"%s\"\n", cfg->syslogHost);
  Serial.printf("  syslogPort: %d\n", cfg->syslogPort);
  Serial.printf("  otaHost: \"%s\"\n", cfg->otaHost);
  Serial.printf("  otaPort: %d\n", cfg->otaPort);
  Serial.printf("  otaUrlBase: \"%s\"\n", cfg->otaUrlBase);
  Serial.printf("  autoFirmwareUpdate: %d\n", cfg->autoFirmwareUpdate);
  Serial.printf("  defaultDevice: %d\n", cfg->defaultDevice);
  Serial.printf("  defaultActive: %d\n", cfg->defaultActive);
  Serial.printf("  displayTimeout: %d\n", cfg->displayTimeout);
  Serial.printf("  alertTime: %d\n", cfg->alertTime);
  Serial.printf("  infoTime: %d\n", cfg->infoTime);
  Serial.printf("  mqttUpdateTime: %d\n", cfg->mqttUpdateTime);
  Serial.printf("  logLevelUart: %d\n", cfg->logLevelUart);
  Serial.printf("  logLevelSyslog: %d\n", cfg->logLevelSyslog);
  Serial.printf("  checksum: 0x%08X\n", cfg->checksum);
}  

void dumpConfigJson(config_t * cfg, char * msg) { 
  Serial.println(msg);
  Serial.println("{");
  Serial.printf("  \"hostname\": \"%s\",\n", cfg->hostname);
  Serial.printf("  \"mqtthost\": \"%s\",\n", cfg->mqttHost);
  Serial.printf("  \"mqttPort\": %d,\n", cfg->mqttPort);
  Serial.printf("  \"mqttUser\": \"%s\",\n", cfg->mqttUser);
  Serial.printf("  \"mqttPswd\": \"%s\",\n", cfg->mqttPswd);
  Serial.printf("  \"mqttBufferSize\": %d,\n", cfg->mqttBufferSize);
  Serial.printf("  \"syslogHost\": \"%s\",\n", cfg->syslogHost);
  Serial.printf("  \"syslogPort\": %d,\n", cfg->syslogPort);
  Serial.printf("  \"otaHost\": \"%s\",\n", cfg->otaHost);
  Serial.printf("  \"otaPort\": %d,\n", cfg->otaPort);
  Serial.printf("  \"otaUrlBase\": \"%s\",\n", cfg->otaUrlBase);
  Serial.printf("  \"autoFirmwareUpdate\": %d,\n", cfg->autoFirmwareUpdate);
  Serial.printf("  \"defaultDevice\": %d,\n", cfg->defaultDevice);
  Serial.printf("  \"defaultActive\": %d,\n", cfg->defaultActive);
  Serial.printf("  \"displayTimeout\": %d,\n", cfg->displayTimeout);
  Serial.printf("  \"alertTime\": %d,\n", cfg->alertTime);
  Serial.printf("  \"infoTime\": %d,\n", cfg->infoTime);
  Serial.printf("  \"mqttUpdateTime\": %d,\n", cfg->mqttUpdateTime);
  Serial.printf("  \"logLevelUart\": %d,\n", cfg->logLevelUart);
  Serial.printf("  \"logLevelSyslog\": %d\n", cfg->logLevelSyslog);
  Serial.println("}");
}  

void dumpConfig(void) { 
  uint16_t i, col;
  uint8_t *buffer = (uint8_t *) &config;
  char ascii[19];
  i = 0;
  col = 0;
  
  Serial.printf("\nconfig(%d bytes):", sizeof(config_t));
  for (i = 0; i < sizeof(config_t); i++) {
    if (col == 0)  Serial.printf("\n%04X: ", i);
    Serial.printf("%02X ", buffer[i]);
    if (buffer[i] < 0x20)  {
      ascii[col] = ' ';
    } else if (buffer[i] > 127) {
      ascii[col] = '.';
    } else {
      ascii[col] = buffer[i];
    }  
    ascii[col+1] = 0; 
    if (++col >= CFG_COLS) {
      col = 0;
      Serial.printf("  %s", ascii);
    }
  }  
  if (col > 0) Serial.printf("  %s\n", ascii);
}

void dumpEEPROMConfig(void) { 
  uint16_t i, col;
  char ascii[19];
  char c;
  i = 0;
  col = 0;
  Serial.printf("\nEEPROM config(%d bytes):", sizeof(config_t));
  EEPROM.begin(sizeof(config_t));
  for (i = 0; i < sizeof(config_t); i++) {
    if (col == 0)  Serial.printf("\n%04X: ", i);
    c = EEPROM.read(i);
    Serial.printf("%02X ", c);
    if (c < 0x20) {
      ascii[col] = ' ';
    } else if (c > 127) {
      ascii[col] = '.';
    } else {
      ascii[col] = c;
    }  
    ascii[col+1] = 0; 
    if (++col >= CFG_COLS) {
      col = 0;
      Serial.printf("  %s", ascii);
    }
  } 
  if (col > 0) Serial.printf("  %s\n", ascii);
  EEPROM.end();
}  


#endif

