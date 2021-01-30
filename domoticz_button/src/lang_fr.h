/* Locale support for strings displayed on the OLED display */

#define LANG "fr"

/* * * main.cpp * * */

// function displayDevice()
#define SC_BM_DIM_LEVEL "< %d >" 
#define SC_BM_SELECTOR "< %s >"
#define SC_BM_DEVICE_DIMMER "%s @ %d%%"
#define SC_BM_DEVICE_SELECTOR "%s"
#define SC_BM_DEVICE_OTHER "%s"

// function displayConfiguration()
#define SC_CO_CONFIGURATION "--Configuration--"
#define SC_CO_FIRMWARE_UPDATE1 "Télécharger"
#define SC_CO_FIRMWARE_UPDATE2 "micrologiciel"
#define SC_CO_CONFIG_UPDATE1 "Télécharger"
#define SC_CO_CONFIG_UPDATE2 "paramètres"
#define SC_CO_DEFAULT_CONFIG1 "Paramètres"
#define SC_CO_DEFAULT_CONFIG2 "par défaut"
#define SC_CO_CLEAR_WIFI1 "Nouveau"
#define SC_CO_CLEAR_WIFI2 "Wi-Fi"
#define SC_CO_SHOW_INFO1 "Montrer"
#define SC_CO_SHOW_INFO2 "information"
#define SC_CO_RESTART1 "Redémarrer"
#define SC_CO_RESTART2 ""

// function doRestart()
#define SC_RESTARTING "Redémarrage..." 

// functino WiFiManagerCallback()
#define SC_ACCESS_POINT "Point d'accès"

// mqttReconnect()
#define SC_MQTT_CONNECTED0 "Connecté au"
#define SC_MQTT_CONNECTED1 "serveur MQTT"
#define SC_MQTT_CONNECTED2 "Mise à jour..."

#define SC_MQTT_NOT_CONNECTED0 "Déconnecté du"
#define SC_MQTT_NOT_CONNECTED1 "serveur MQTT"
#define SC_MQTT_NOT_CONNECTED2 "..."

// setup()
#define SC_FIRMWARE_VERSION "version"

#define SC_WIFI_CONNECTED0 "Connecté"
// SC_WIFI_CONNECTED1 STAT_IP
#define SC_WIFI_CONNECTED2 "Micrologiciel ?"

#define SC_FIRMWARE_LOADED0 "Nouveau"
#define SC_FIRMWARE_LOADED1 "micrologiciel"
#define SC_FIRMWARE_LOADED2 "téléchargé"  

#define SC_FIRMWARE_FAIL0 "Échec en"
#define SC_FIRMWARE_FAIL1 "chargeant le"
#define SC_FIRMWARE_FAIL2 "micrologiciel"


/* * * devices.cpp * * */

#define SC_ZONE0 "Étage"
#define SC_ZONE1 "Rez-de-chaussée"
#define SC_ZONE2 "Sous-sol"
#define SC_ZONE3 "Garage"
#define SC_ZONE4 "Maison"

#define SC_STATUS_OFF "·"
#define SC_STATUS_ON "¤"
#define SC_STATUS_MIXED "·/¤"
#define SC_STATUS_NO "Non"
#define SC_STATUS_YES "Oui"
#define SC_STATUS_CLOSED "Fermée"
#define SC_STATUS_OPEN "Ouverte"
#define SC_STATUS_TIMER_PLAN0 "Normal"
#define SC_STATUS_TIMER_PLAN1 "Absent"
#define SC_STATUS_TIMER_PLAN2 "Vacances"



