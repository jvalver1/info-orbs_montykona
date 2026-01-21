#ifndef DEBUG_HELPER_H
#define DEBUG_HELPER_H

#include <Arduino.h>

// Forward declare the MainHelper class to avoid circular dependency
class MainHelper;

// Declare the global isDebugEnabled function that will call MainHelper::isDebugEnabled()
extern bool isDebugEnabled_global();

// Debug macros that check the global debug flag before outputting
#define DEBUG_PRINT(msg) \
    do { \
        if (isDebugEnabled_global()) { \
            Serial.print(msg); \
        } \
    } while(0)

#define DEBUG_PRINTLN(msg) \
    do { \
        if (isDebugEnabled_global()) { \
            Serial.println(msg); \
        } \
    } while(0)

#define DEBUG_PRINTF(fmt, ...) \
    do { \
        if (isDebugEnabled_global()) { \
            Serial.printf(fmt, ##__VA_ARGS__); \
        } \
    } while(0)

#define DEBUG_WRITE(data) \
    do { \
        if (isDebugEnabled_global()) { \
            Serial.write(data); \
        } \
    } while(0)

#endif // DEBUG_HELPER_H
