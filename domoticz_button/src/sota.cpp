
// based on 
// Self-updating OTA firmware for ESP8266 by Erik H. Bakke (OppoverBakke)
// https://www.bakke.online/index.php/2017/06/02/self-updating-ota-firmware-for-esp8266/


#include <Arduino.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <WiFiClient.h>
#include "config.h"
#include "logging.h"
#include "sota.h"

WiFiClient wifiClient;

//#define USE_MAC

#ifdef USE_MAC

String getMAC() {
  uint8_t mac[6];
  char result[14];
  snprintf( result, sizeof( result ), "%02x%02x%02x%02x%02x%02x", mac[ 0 ], mac[ 1 ], mac[ 2 ], mac[ 3 ], mac[ 4 ], mac[ 5 ] );
  return String( result );
}

#endif

OTA_result_t checkForUpdates(void) {
  OTA_result_t result = OTA_FAILED;
  #ifdef USE_MAC
  String mac = getMAC();
  sendToLogPf(LOG_INFO, PSTR("Checking for firmware updates for %s"),  mac.c_str());
  #else
  String mac = config.hostname;
  #endif
  String imageURL = String("http://") + config.otaHost + ':' + config.otaPort + config.otaUrlBase + mac; 
  String versionURL = imageURL + ".version";
  imageURL.concat(".bin");
  
  HTTPClient httpClient;

  httpClient.begin(wifiClient, versionURL);
  int httpCode = httpClient.GET();
  if (httpCode == 404) {
    sendToLogPf(LOG_INFO, PSTR("%s.%s not found on OTA server"), mac.c_str(), "version");
  } else if (httpCode != 200) {
    sendToLogPf(LOG_INFO, PSTR("Firmware version check failed. HTTP response code %d"), httpCode);
  } else { 
    String newFWVersion = httpClient.getString();
    int newVersion = newFWVersion.toInt();
    //sendToLogPf(LOG_INFO, PSTR("Current firmware version %d, available version %d"), VERSION, newVersion);

    if (newVersion <= VERSION) {
      sendToLogPf(LOG_INFO, PSTR("Current firmware version %d is the latest version"), VERSION);
      result = OTA_NO_NEW_VERSION;
    } else {  
      HTTPUpdateResult ret = ESPhttpUpdate.update(imageURL);
      switch (ret) {
          case HTTP_UPDATE_FAILED:
            sendToLogPf(LOG_ERR, PSTR("Firmware update to version %d failed with error (%d): %s"), newVersion, ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
            break;

          case HTTP_UPDATE_NO_UPDATES:
            sendToLogP(LOG_ERR, PSTR("Firmware update to version %d failed: NO UPDATES"));
            break;

          case HTTP_UPDATE_OK:
            sendToLogPf(LOG_INFO, PSTR("Updated firmware from version %d to version %d"), VERSION, newVersion);
            result = OTA_NEW_VERSION_LOADED;
            break;
      }
    }
  }
  httpClient.end();
  return result;
}

