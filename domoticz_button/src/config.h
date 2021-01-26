
#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <cstdint>
#include "logging.h"

//#define DEBUG_CONFIG

#define APP_NAME   "Domoticz button"
#define VERSION    0x000103 // *** Make sure that the integer contained in the *.version file ***
                            // *** served by the OTA Web server matches this VERSION number. ***

// *** Hostname ***
// 
// Allowed chars: "a".."z", "A".."Z", "0".."9" "-", but must not start nor end with "-". 
// Assume that the name is not case sensitive. Ref: https://en.wikipedia.org/wiki/Hostname
//
// The host name is used to identify the device with the MQTT broker 
// and used to get firmware update from the OTA server

#define HOST_NAME   "DomoButton-1"

// *** Syslog server ***

#define SYSLOG_HOST "192.168.1.11"
#define SYSLOG_PORT 514

// *** MQTT server ***

#define MQTT_HOST "192.168.1.11"
#define MQTT_PORT 1883
#define MQTT_USER ""
#define MQTT_PSWD ""
#define MQTT_BUFFER_SIZE 768;

// *** OTA server ***
//
// The URL of the file containing the currently available version number will be
//    http:// + config.otaHost + ":" + config.otaPort + config.otaUrlBase + config.hostname + ".version"
// while the URL of the file containting the firmware will be
//    http:// + config.otaHost + ":" + config.otaPort + config.otaUrlBase + config.hostname + ".bin"

#define OTA_HOST "192.168.1.11"
#define OTA_PORT 80
#define OTA_URL_BASE  "/domoticz_button/"  // include trailing separator


// *** Time delays (in seconds) ***

#define DISPLAY_TIMEOUT    15  // period of inactivity before blanking display
#define ALERT_TIME          3  // alert on/off time
#define MQTT_UPDATE_TIME    5  // initial updating time after MQTT subscribe in mqttReconnect()
#define INFO_TIME           3  // minimum time displaying statup info messages 

// *** Log levels ***

#define LOG_LEVEL_UART    LOG_DEBUG
#define LOG_LEVEL_SYSLOG  LOG_ERR

// *** Domoticz MQTT topics 

#define DOMO_SUB_TOPIC "domoticz/in"   // case sensitive
#define DOMO_PUB_TOPIC "domoticz/out"  // case sensitive

// *** String buffer lengths

// Maximum number of characters in string including terminating 0
//
#define URL_SZ             81  
#define SSID_SZ            33  
#define IP_SZ              17  // 255.255.255.255
#define PSWD_SZ            65  // minimum 8
#define HOST_NAME_SZ       32  // maximum size of 31 bytes for OpenSSL e-mail certificates
#define MSG_SZ            441  // needs to be big enough for "reach" command (i.e. > 3*URL_SZ) 
#define TOPIC_SZ       PWD_SZ

#define CONFIG_MAGIC    0x4D44    // 'M'+'D'

struct config_t {
  uint16_t magic;                 // check for valid config 
  char hostname[HOST_NAME_SZ];    // used for Wi-Fi connection and MQTT broker connection
  char mqttHost[URL_SZ];          // URL of MQTT server 
  uint16_t mqttPort;              // MQTT port
  char mqttUser[SSID_SZ];         // *** NOT YET IMPLEMENTED ***
  char mqttPswd[PSWD_SZ];         // *** NOT YET IMPLEMENTED ***
  uint16_t mqttBufferSize;     
  char syslogHost[URL_SZ];        // URL of Syslog server
  uint16_t syslogPort;            // Syslog port
  char otaHost[URL_SZ];
  uint16_t otaPort;
  char otaUrlBase[HOST_NAME_SZ];
  uint32_t displayTimeout;
  uint32_t alertTime; 
  uint32_t infoTime;
  uint32_t mqttUpdateTime;
  uint8_t logLevelUart;
  uint8_t logLevelSyslog;  
  uint32_t checksum;              // Used to validate saved configuration
};

typedef enum {
  LC_FROM_EEPROM,
  LC_FROM_DEFAULTS
} loadConfig_t;

loadConfig_t loadConfig(void); 
void useDefaultConfig(void);
bool updateConfig(void);
void clearEEPROM(void);  // default config will be used on next boot

#ifdef DEBUG_CONFIG
  uint32_t getConfigHash();
  uint32_t loadConfigFromEEPROM(void);
  void useDefaultConfig(void);
  void clearConfig(config_t* cfg);
  void saveConfigToEEPROM(void);

  void dumpConfig(config_t* cfg, char* msg = NULL);
  void dumpConfig(void);
  void dumpEEPROMConfig(void);
#endif  

extern config_t config;  // defined in config.cpp

#endif
