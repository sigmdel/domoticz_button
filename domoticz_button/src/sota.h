#ifndef SOTA_H
#define SOTA_H

// Self updating over the air

// Types of Domoticz virtual devices that will be handled 
enum OTA_result_t { 
   OTA_NO_NEW_VERSION,
   OTA_NEW_VERSION_LOADED,
   OTA_FAILED
};

OTA_result_t checkForUpdates(void);

#endif