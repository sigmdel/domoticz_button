#include <Arduino.h>
#include <cstdint>
#include <EEPROM.h>

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

  strlcpy(config.syslogHost, SYSLOG_HOST, SSID_SZ);
  config.syslogPort = SYSLOG_PORT;
  
  config.displayTimeout = DISPLAY_TIMEOUT;
  config.alertTime = ALERT_TIME;

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
int loadConfig(void) {
  eepromConfigHash = loadConfigFromEEPROM();
  if (config.magic != CONFIG_MAGIC || eepromConfigHash != config.checksum ) {
    /*
    if (config.magic != CONFIG_MAGIC)
      sendToLogP(LOG_DEBUG, PSTR("Config magic id wrong"));
    if (eepromConfigHash != config.checksum) 
      sendToLogPf(LOG_DEBUG, "Saved config checksum: 0x%x08, calculated: 0x%x08", config.checksum, eepromConfigHash);
    */
    useDefaultConfig();
    eepromConfigHash = config.checksum;
    return 1;
  }
  return 0;
}

void clearEEPROM(void) {
  EEPROM.begin(sizeof(uint16_t));
  for (uint16_t i = 0 ; i < sizeof(uint16_t) ; i++) {
    EEPROM.write(i, 0xFF);
  }
  EEPROM.end();  
  sendToLogP(LOG_DEBUG, PSTR("Cleared configuration in EEPROM")); 
}
