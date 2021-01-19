#include <Arduino.h>
#include <cstdint>
#include "config.h"
#include "logging.h"

config_t config;

void useDefaultConfig(void) {
  memset(&config, 0x00, sizeof(config_t));
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
  
  config.displayTimeout = DISPLAY_TIMEOUT*1000;
  config.alertTime = ALERT_TIME*1000;
  config.restartMsgTime = RESTART_MSG_TIME*1000;
  config.mqttUpdateTime = MQTT_UPDATE_TIME*1000;
  config.infoTime = INFO_TIME*1000;

  config.logLevelUart = LOG_LEVEL_UART;
  config.logLevelSyslog = LOG_LEVEL_SYSLOG;
} 

int loadConfig(void) {
  useDefaultConfig();
  return 1;
}
