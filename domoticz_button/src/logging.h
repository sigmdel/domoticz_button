#ifndef _LOGGING_H_
#define _LOGGING_H_

#include <Arduino.h>

/*
 * A logging facililty
 */

/*
 * Log priority levels "borrowed" from syslog
 */

enum Log_level {
   LOG_EMERG = 0,  /* system is unusable, index = 0 just to be sure*/
   LOG_ALERT,      /* action must be taken immediately */
   LOG_CRIT,       /* critical conditions */
   LOG_ERR,        /* error conditions */
   LOG_WARNING,    /* warning conditions */
   LOG_NOTICE,     /* normal but significant condition */
   LOG_INFO,       /* informational */
   LOG_DEBUG,      /* debug-level messages */
   LOG_LEVEL_COUNT /* number of log levels */
};

extern const char *logLevelString[LOG_LEVEL_COUNT];

/*
 * A string or strings sent to the log with any of the logging functions will be 
 *  a) printed to the serial port if level has a higher or equal priority to serialThreshold
 *  b) sent to the web server if level has a higher or equal priority to webThreshold
 *  c) sent to the syslog server if level has a higher or equal priority to syslogThreshold 
 * 
 * To see the syslog messages as they come in on the systel log server oldpi.local
 *    pi@oldpi:~ $ tail -f /var/log/syslog
 */

  // Typical use: sendToLog(LOG_INFO, "Some information");
void sendToLog(Log_level level, const char *line );

  // Typical use: sendToLogf(LOG_INFO, "Count: %d, free: %d at %s", 32, 12498, "some_string");
void sendToLogf(Log_level level, const char *format, ...);

  // Typical use: sendToLogP(LOG_ERR, PSTR("Fatal Error"));
void sendToLogP(Log_level level, const char *line);

  // Typical use: sendToLogP(LOG_ERR, PSTR("Fatal Error"), PSTR("REBOOTING"));
void sendToLogP(Log_level level, const char *linep1, const char *linep2);

  // Typical use: sendToLogPf(LOG_INFO, PSTR("Count: %d, free: %d at %s"), 32, 12498, "some_string");
void sendToLogPf(Log_level level, const char *pline, ...);

void mstostr(unsigned long milli, char* sbuf, int sbufsize);



#endif
