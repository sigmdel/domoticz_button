
#include <Arduino.h>
#include <WiFiUdp.h>
#include <ESP8266WiFi.h>
#include <cstdint>
#include "config.h"
#include "logging.h"

void mstostr(unsigned long milli, char* sbuf, int sbufsize) {
  int sec = milli / 1000;
  int min = sec / 60;
  int hr = min / 60;
  min = min % 60;
  sec = sec % 60;
  int frac = (milli % 1000);
  snprintf_P(sbuf, sbufsize-1, PSTR("%02d:%02d:%02d.%03d"), hr,min,sec,frac);
}

#define LOGSZ 520

WiFiUDP udp;

extern config_t config;

const char *logLevelString[LOG_LEVEL_COUNT] = {
  "emerg",  
  "alert",
  "crit",
  "err",
  "warning",
  "notice",
  "info",
  "debug"
};

void sendToLog(Log_level level, const char *line ) {
  //dbg: Serial.printf("\nSendToLog(%d, %s)\n", level, line);
  String message;
  char mxtime[15];

  // message set to time stamp + line
  mstostr(millis(), mxtime, sizeof(mxtime));
  message = String(mxtime) + " " + String(line);

  if (level <= config.logLevelUart) {
    Serial.printf("%s\n", message.c_str());
  } 

  if (level <= config.logLevelSyslog)  { 
    // This does not block if WiFi is not present or not connected
    message = String(config.hostname) + ": " + String(line);
    #ifdef DEBUG_LOG
      Serial.printf("Sending %s to syslog %s:%d\n", message.c_str(), config.syslogHost, config.syslogPort);
    #endif  
    if (udp.beginPacket(config.syslogHost, config.syslogPort)) {
      udp.write(message.c_str());
      udp.endPacket();
      delay(1);  // Add time for UDP handling 
    }  
    //Serial.println("...done");  
  }

}

void sendToLogf(Log_level level, const char *format, ...) {
  va_list args;
  char line[250];
  va_start(args, format);
  vsnprintf(line, 250, format, args);
  va_end(args);
  sendToLog(level, line);
}

void sendToLogP(Log_level level, const char *line) {
  char msg[LOGSZ];
  strcpy_P(msg, line);
  sendToLog(level, msg);
}  

void sendToLogP(Log_level level, const char *linep1, const char *linep2) {
  char msg[LOGSZ];
  char msg2[LOGSZ];
  strcpy_P(msg, linep1);
  strcpy_P(msg2, linep2);
  strncat(msg, msg2, sizeof(msg));
  sendToLog(level, msg);
}

void sendToLogPf(Log_level level, const char *pline, ...) {
  va_list args;
  char msg[LOGSZ];
  char msg2[LOGSZ];
  strcpy_P(msg, pline);
  va_start(args, pline);
  vsnprintf(msg2, sizeof(msg2), msg, args);
  va_end(args);
  sendToLog(level, msg2);
}

