#ifndef SHOW_MEMORY_USAGE_H
#define SHOW_MEMORY_USAGE_H

#include "DebugHelper.h"

#ifdef MEMORY_DEBUG_INTERVAL
    #define SHOW_MEMORY_USAGE(msg)                 \
        do {                                       \
            DEBUG_PRINT(" --- ");                  \
            DEBUG_PRINTLN(msg);                    \
            ShowMemoryUsage::printSerial(false);   \
            DEBUG_PRINTLN();                       \
        } while (0)
#else
    #define SHOW_MEMORY_USAGE(msg) // No-op
#endif

class ShowMemoryUsage {
public:
    static void printSerial(bool force = false, bool newLine = true);
};

#endif // SHOW_MEMORY_USAGE_H
