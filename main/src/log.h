#ifndef LOG_H
#define LOG_H

#include <Arduino.h>
#include "heltec.h"

enum LogLevelCode {
    LVL_SILENT = 0,
    LVL_TEST   = 1,
    LVL_INFO   = 2,
    LVL_DEBUG  = 3
};

class logger
{
private:
    LogLevelCode currentLevel;
    const char *id;
    const bool displayOn;
    
    LogLevelCode parseLevel(const char *level);
    void preLog();

public:
    // VIGTIGT: to parametre, så det matcher log.cpp
    logger(const char *logId, const char *level);
    ~logger();

    bool returnLevel(const char *level);
    static void display(const char *msg);

    // log (uden newline)
    void log(const char *msg, const char *level, bool metaLog);
    void log(int msg,        const char *level, bool metaLog);
    void log(float msg,      const char *level, bool metaLog);
    void log(double msg,     const char *level, bool metaLog);
    void log(float value, int decimals, const char *level, bool metaLog);
    void log(bool value,     const char *level, bool metaLog);

    // logln (med newline)
    void logln(const char *msg, const char *level, bool metaLog);
    void logln(int msg,        const char *level, bool metaLog);
    void logln(float msg,      const char *level, bool metaLog);
    void logln(double msg,     const char *level, bool metaLog);
    void logln(float value, int decimals, const char *level, bool metaLog);
    void logln(bool value,     const char *level, bool metaLog);
};

#endif // LOG_H
