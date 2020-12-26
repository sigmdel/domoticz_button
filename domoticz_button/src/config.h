
#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <cstdint>

#define APP_NAME   "Domoticz button"

// *** Default hostname of device on WiFi network  ***
// 
// Allowed chars: "a".."z", "A".."Z", "0".."9" "-", but must not start nor end with "-". 
// Assume that the name is not case sensitive. Ref: https://en.wikipedia.org/wiki/Hostname

#define HOST_NAME   "DomoButton-1"

// *** Syslog server ***

#define SYSLOG_HOST "192.168.1.11"
#define SYSLOG_PORT 514

#define MQTT_HOST "192.168.1.11"
#define MQTT_PORT 1883
#define MQTT_USER ""
#define MQTT_PSWD ""
#define MQTT_BUFFER_SIZE 768;

#define DISPLAY_TIMEOUT 15*1000  // ms of inactivity blanks display
#define ALERT_TIME       3*1000  // alert flash time

// *** Log levels ***

#define LOG_LEVEL_UART    LOG_DEBUG
#define LOG_LEVEL_SYSLOG  LOG_ERR


//==========//==========//==========//==========//==========//==========//==========//
// Everything above are default values and may be modified with commands in the future.
// Everything below should be set once for all and cannot be modified at run time.
//==========//==========//==========//==========//==========//==========//==========//

#define VERSION    0x000100  // version 0.1.0

#define DOMO_SUB_TOPIC "domoticz/in"   // case sensitive
#define DOMO_PUB_TOPIC "domoticz/out"  // case sensitive

// Maximum number of characters in string including terminating 0
//
#define URL_SZ             81  
#define SSID_SZ            33  
#define IP_SZ              17  // 255.255.255.255
#define PSWD_SZ            65  // minimum 8
#define HOST_NAME_SZ       32  // maximum size of 31 bytes for OpenSSL e-mail certificates
#define MSG_SZ            441  // needs to be big enough for "reach" command (i.e. > 3*URL_SZ) 
#define TOPIC_SZ       PWD_SZ

#define CONFIG_MAGIC    0x4D45    // 'M'+'D'

struct config_t {
  uint16_t magic;                 // check for valid config 
  char hostname[HOST_NAME_SZ];    // used for Wi-Fi connection and MQTT broker connection
  char mqttHost[URL_SZ];          // URL of MQTT server 
  uint16_t mqttPort;              // MQTT port
  char mqttUser[SSID_SZ];
  char mqttPswd[PSWD_SZ];
  uint16_t mqttBufferSize;
  char syslogHost[URL_SZ];        // URL of Syslog server
  uint16_t syslogPort;            // Syslog port
  uint32_t displayTimeout;
  uint32_t alertTime; 
  uint8_t logLevelUart;
  uint8_t logLevelSyslog;  
  uint32_t checksum;
};

void useDefaultConfig(void);
void saveConfigToEEPROM(void);
uint32_t loadConfigFromEEPROM(void);
int loadConfig(void);  // 1 - loaded default config, 2 - loaded EEPROM config (default)
void clearEEPROM(void);

extern config_t config;

#endif
