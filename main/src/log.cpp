#include "log.h"
#include <string.h> // strcmp

logger::logger(const char *logId, const char *level)
{
    currentLevel = parseLevel(level);
    id = logId;
}

logger::~logger()
{
}

LogLevelCode logger::parseLevel(const char *level)
{
    if (level == nullptr)
        return LVL_SILENT;

    if (strcmp(level, "SILENT") == 0)
        return LVL_SILENT;
    if (strcmp(level, "TEST") == 0)
        return LVL_TEST;
    if (strcmp(level, "INFO") == 0)
        return LVL_INFO;
    if (strcmp(level, "DEBUG") == 0)
        return LVL_DEBUG;

    return LVL_SILENT;
}

void logger::preLog()
{
    unsigned long timestamp = millis();
    Serial.print("millis: ");
    Serial.print(timestamp);
    Serial.print(": ");
    Serial.print(id);
    Serial.print(" ");
    Serial.print(" -> ");
}

static bool shouldLog(LogLevelCode currentLevel, LogLevelCode msgLevel)
{
    return msgLevel <= currentLevel;
}

bool logger::returnLevel(const char *level)
{
    LogLevelCode msgLevel = parseLevel(level);
    return !shouldLog(currentLevel, msgLevel);
}

// -------- log (uden newline) --------
void logger::log(const char *msg, const char *level, bool metaLog)
{
    LogLevelCode msgLevel = parseLevel(level);
    if (!shouldLog(currentLevel, msgLevel))
        return;

    if (metaLog)
        preLog();
    Serial.print(msg);
}

void logger::log(int msg, const char *level, bool metaLog)
{
    LogLevelCode msgLevel = parseLevel(level);
    if (!shouldLog(currentLevel, msgLevel))
        return;

    if (metaLog)
        preLog();
    Serial.print(msg);
}

void logger::log(float msg, const char *level, bool metaLog)
{
    LogLevelCode msgLevel = parseLevel(level);
    if (!shouldLog(currentLevel, msgLevel))
        return;

    if (metaLog)
        preLog();
    Serial.print(msg);
}

void logger::log(double msg, const char *level, bool metaLog)
{
    LogLevelCode msgLevel = parseLevel(level);
    if (!shouldLog(currentLevel, msgLevel))
        return;

    if (metaLog)
        preLog();
    Serial.print(msg);
}

void logger::log(float value, int decimals, const char *level, bool metaLog)
{
    LogLevelCode msgLevel = parseLevel(level);
    if (!shouldLog(currentLevel, msgLevel))
        return;

    if (metaLog)
        preLog();
    Serial.print(value, decimals);
}

void logger::log(bool value, const char *level, bool metaLog)
{
    LogLevelCode msgLevel = parseLevel(level);
    if (!shouldLog(currentLevel, msgLevel))
        return;

    if (metaLog)
        preLog();
    Serial.print(value);
}

// -------- logln (med newline) --------

void logger::logln(const char *msg, const char *level, bool metaLog)
{
    LogLevelCode msgLevel = parseLevel(level);
    if (!shouldLog(currentLevel, msgLevel))
        return;

    if (metaLog)
        preLog();
    Serial.println(msg);
}

void logger::logln(int msg, const char *level, bool metaLog)
{
    LogLevelCode msgLevel = parseLevel(level);
    if (!shouldLog(currentLevel, msgLevel))
        return;

    if (metaLog)
        preLog();
    Serial.println(msg);
}

void logger::logln(float msg, const char *level, bool metaLog)
{
    LogLevelCode msgLevel = parseLevel(level);
    if (!shouldLog(currentLevel, msgLevel))
        return;

    if (metaLog)
        preLog();
    Serial.println(msg);
}

void logger::logln(double msg, const char *level, bool metaLog)
{
    LogLevelCode msgLevel = parseLevel(level);
    if (!shouldLog(currentLevel, msgLevel))
        return;

    if (metaLog)
        preLog();
    Serial.println(msg);
}

void logger::logln(float value, int decimals, const char *level, bool metaLog)
{
    LogLevelCode msgLevel = parseLevel(level);
    if (!shouldLog(currentLevel, msgLevel))
        return;

    if (metaLog)
        preLog();
    Serial.println(value, decimals);
}

void logger::logln(bool value, const char *level, bool metaLog)
{
    LogLevelCode msgLevel = parseLevel(level);
    if (!shouldLog(currentLevel, msgLevel))
        return;

    if (metaLog)
        preLog();
    Serial.println(value);
}