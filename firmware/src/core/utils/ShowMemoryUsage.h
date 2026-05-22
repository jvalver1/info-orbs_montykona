#ifndef SHOW_MEMORY_USAGE_H
#define SHOW_MEMORY_USAGE_H

#include "DebugHelper.h"

#if defined(MEMORY_DEBUG_INTERVAL) && (WIDGET_DEBUG_LEVEL > WIDGET_LOG_LEVEL_SILENT)
    // Rate-limited periodic dump (used in loop())
    #define SHOW_MEMORY_USAGE(msg)                   \
        do {                                         \
            if (isDebugEnabled_global()) {           \
                DEBUG_PRINT(" --- ");                \
                DEBUG_PRINTLN(msg);                  \
                ShowMemoryUsage::printSerial(false); \
                DEBUG_PRINTLN();                     \
            }                                        \
        } while (0)

    // Always-on labeled snapshot — use this at specific instrumentation points.
    // Prints immediately regardless of the rate-limit timer.
    // Format: [HEAP] <label> | free=<n> largest=<n> frag=<n>% blocks=<free>/<total> minFree=<n>
    #define HEAP_SNAP(label)                           \
        do {                                           \
            if (isDebugEnabled_global()) {             \
                ShowMemoryUsage::printDetailed(label); \
            }                                          \
        } while (0)
#else
    #define SHOW_MEMORY_USAGE(msg) // No-op
    #define HEAP_SNAP(label) // No-op
#endif

class ShowMemoryUsage {
public:
    // Rate-limited periodic dump (respects MEMORY_DEBUG_INTERVAL)
    static void printSerial(bool force = false, bool newLine = true);

    // Immediate labeled snapshot — always prints, ignores rate-limit timer.
    // Logs: free, largest free block, fragmentation %, free/total block count, all-time minimum.
    static void printDetailed(const char *label);
};

#endif // SHOW_MEMORY_USAGE_H
