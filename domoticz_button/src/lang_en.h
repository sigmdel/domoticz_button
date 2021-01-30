/* Locale support for strings displayed on the OLED display */

#define LANG "en"

/* * * main.cpp * * */

// function displayDevice()
#define SC_BM_DIM_LEVEL "< %d >" 
#define SC_BM_SELECTOR "< %s >"
#define SC_BM_DEVICE_DIMMER "%s @ %d%%"
#define SC_BM_DEVICE_SELECTOR "%s"
#define SC_BM_DEVICE_OTHER "%s"

// function displayConfiguration()
#define SC_CO_CONFIGURATION "--Configuration--"
#define SC_CO_FIRMWARE_UPDATE1 "Download"
#define SC_CO_FIRMWARE_UPDATE2 "firmware"
#define SC_CO_CONFIG_UPDATE1 "Download"
#define SC_CO_CONFIG_UPDATE2 "options"
#define SC_CO_DEFAULT_CONFIG1 "Use default"
#define SC_CO_DEFAULT_CONFIG2 "options"
#define SC_CO_CLEAR_WIFI1 "Clear"
#define SC_CO_CLEAR_WIFI2 "Wi-Fi"
#define SC_CO_SHOW_INFO1 "Show"
#define SC_CO_SHOW_INFO2 "information"
#define SC_CO_RESTART1 "Restart"
#define SC_CO_RESTART2 ""

// function doRestart()
#define SC_RESTARTING "Restarting..." 

// functino WiFiManagerCallback()
#define SC_ACCESS_POINT "Access Point"

// mqttReconnect()
#define SC_MQTT_CONNECTED0 "Connected to"
#define SC_MQTT_CONNECTED1 "MQTT broker"
#define SC_MQTT_CONNECTED2 "Updating..."

#define SC_MQTT_NOT_CONNECTED0 "Not connected"
#define SC_MQTT_NOT_CONNECTED1 "to MQTT broker"
#define SC_MQTT_NOT_CONNECTED2 "..."

// setup()
#define SC_FIRMWARE_VERSION "version"

#define SC_WIFI_CONNECTED0 "Connected as"
// SC_WIFI_CONNECTED1 STA_IP
#define SC_WIFI_CONNECTED2 "Firmware check..."

#define SC_FIRMWARE_LOADED0 "New firmware"
#define SC_FIRMWARE_LOADED1 "loaded" 
#define SC_FIRMWARE_LOADED2 "" 

#define SC_FIRMWARE_FAIL0 "Failed to load"
#define SC_FIRMWARE_FAIL1 "new firmware"
#define SC_FIRMWARE_FAIL2 ""

/* * * devices.cpp * * */

#define SC_ZONE0 "Upstairs"
#define SC_ZONE1 "Ground level"
#define SC_ZONE2 "Downstairs"
#define SC_ZONE3 "Garage"
#define SC_ZONE4 "House"

#define SC_STATUS_OFF "Off"
#define SC_STATUS_ON "On"
#define SC_STATUS_MIXED "Mixed"
#define SC_STATUS_NO "No"
#define SC_STATUS_YES "Yes"
#define SC_STATUS_CLOSED "Closed"
#define SC_STATUS_OPEN "Open"
#define SC_STATUS_TIMER_PLAN0 "Default"
#define SC_STATUS_TIMER_PLAN1 "Weekend"
#define SC_STATUS_TIMER_PLAN2 "Holidays"



