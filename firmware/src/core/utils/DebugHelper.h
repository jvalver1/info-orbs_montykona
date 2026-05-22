#ifndef DEBUG_HELPER_H
#define DEBUG_HELPER_H

#include <Arduino.h>

// Forward declare the MainHelper class to avoid circular dependency
class MainHelper;

// Declare the global isDebugEnabled function that will call MainHelper::isDebugEnabled()
extern bool isDebugEnabled_global();

// Debug macros that check the global debug flag before outputting
#define DEBUG_PRINT(msg)               \
    do {                               \
        if (isDebugEnabled_global()) { \
            Serial.print(msg);         \
        }                              \
    } while (0)

#define DEBUG_PRINTLN(msg)             \
    do {                               \
        if (isDebugEnabled_global()) { \
            Serial.println(msg);       \
        }                              \
    } while (0)

#define DEBUG_PRINTF(fmt, ...)                 \
    do {                                       \
        if (isDebugEnabled_global()) {         \
            Serial.printf(fmt, ##__VA_ARGS__); \
        }                                      \
    } while (0)

#define DEBUG_WRITE(data)              \
    do {                               \
        if (isDebugEnabled_global()) { \
            Serial.write(data);        \
        }                              \
    } while (0)

// --- Widget Specific Unified Logging Framework ---

#define WIDGET_LOG_LEVEL_SILENT 0
#define WIDGET_LOG_LEVEL_ERROR 1
#define WIDGET_LOG_LEVEL_WARN 2
#define WIDGET_LOG_LEVEL_INFO 3
#define WIDGET_LOG_LEVEL_TRACE 4

// Set default debug level if not defined in config.h
#ifndef WIDGET_DEBUG_LEVEL
    #define WIDGET_DEBUG_LEVEL WIDGET_LOG_LEVEL_INFO
#endif

#if WIDGET_DEBUG_LEVEL >= WIDGET_LOG_LEVEL_ERROR
    #define WIDGET_LOG_ERROR(prefix, fmt, ...)                               \
        do {                                                                 \
            if (isDebugEnabled_global()) {                                   \
                Serial.printf("%s ERROR: " fmt "\n", prefix, ##__VA_ARGS__); \
            }                                                                \
        } while (0)
#else
    #define WIDGET_LOG_ERROR(prefix, fmt, ...) \
        do {                                   \
        } while (0)
#endif

#if WIDGET_DEBUG_LEVEL >= WIDGET_LOG_LEVEL_WARN
    #define WIDGET_LOG_WARN(prefix, fmt, ...)                                  \
        do {                                                                   \
            if (isDebugEnabled_global()) {                                     \
                Serial.printf("%s WARNING: " fmt "\n", prefix, ##__VA_ARGS__); \
            }                                                                  \
        } while (0)
#else
    #define WIDGET_LOG_WARN(prefix, fmt, ...) \
        do {                                  \
        } while (0)
#endif

#if WIDGET_DEBUG_LEVEL >= WIDGET_LOG_LEVEL_INFO
    #define WIDGET_LOG_INFO(prefix, fmt, ...)                         \
        do {                                                          \
            if (isDebugEnabled_global()) {                            \
                Serial.printf("%s " fmt "\n", prefix, ##__VA_ARGS__); \
            }                                                         \
        } while (0)
#else
    #define WIDGET_LOG_INFO(prefix, fmt, ...) \
        do {                                  \
        } while (0)
#endif

#if WIDGET_DEBUG_LEVEL >= WIDGET_LOG_LEVEL_TRACE
    #define WIDGET_LOG_TRACE(prefix, fmt, ...)                               \
        do {                                                                 \
            if (isDebugEnabled_global()) {                                   \
                Serial.printf("%s TRACE: " fmt "\n", prefix, ##__VA_ARGS__); \
            }                                                                \
        } while (0)
    #define WIDGET_LOG_TRACE_RAW(fmt, ...)         \
        do {                                       \
            if (isDebugEnabled_global()) {         \
                Serial.printf(fmt, ##__VA_ARGS__); \
            }                                      \
        } while (0)
#else
    #define WIDGET_LOG_TRACE(prefix, fmt, ...) \
        do {                                   \
        } while (0)
    #define WIDGET_LOG_TRACE_RAW(fmt, ...) \
        do {                               \
        } while (0)
#endif

// Labeled snapshot of heap memory, showing free memory, largest free block, and fragmentation percentage.
// Respects both compile-time WIDGET_DEBUG_LEVEL (compiled out if SILENT) and runtime isDebugEnabled_global().
#if WIDGET_DEBUG_LEVEL > WIDGET_LOG_LEVEL_SILENT
    #include <esp_heap_caps.h>
    #define WIDGET_HEAP_SNAP(prefix, label)                                                                 \
        do {                                                                                                \
            if (isDebugEnabled_global()) {                                                                  \
                multi_heap_info_t info;                                                                     \
                heap_caps_get_info(&info, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);                           \
                size_t totalFree = info.total_free_bytes;                                                   \
                size_t largest = info.largest_free_block;                                                   \
                uint8_t fragPct = (totalFree > 0) ? (uint8_t) (100u - (largest * 100u / totalFree)) : 100u; \
                Serial.printf("%s [HEAP] %s | free=%d  largest=%d  frag=%d%%\n",                            \
                              prefix,                                                                       \
                              label ? label : "",                                                           \
                              (int) totalFree,                                                              \
                              (int) largest,                                                                \
                              (int) fragPct);                                                               \
            }                                                                                               \
        } while (0)
#else
    #define WIDGET_HEAP_SNAP(prefix, label) \
        do {                                \
        } while (0)
#endif

#endif // DEBUG_HELPER_H
