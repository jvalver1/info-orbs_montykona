#include "ShowMemoryUsage.h"

#include "DebugHelper.h"
#include "config_helper.h"
#include <Arduino.h>

// initialize static members
static unsigned long s_lastMemoryUsageShownAt = 0;

#ifdef MEMORY_DEBUG_INTERVAL
static const unsigned long interval_ms = MEMORY_DEBUG_INTERVAL;
#else
static const unsigned long interval_ms = 5000; // default is needed for cases of where this method is called without MEMORY_DEBUG_INTERVAL being set
#endif

void ShowMemoryUsage::printSerial(bool force, bool newLine) {
    multi_heap_info_t info;
    if (force || (!s_lastMemoryUsageShownAt || (millis() - s_lastMemoryUsageShownAt >= interval_ms))) {
        heap_caps_get_info(&info, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        size_t total = heap_caps_get_total_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        size_t totalFree = info.total_free_bytes;
        size_t largest = info.largest_free_block;
        // Diag 4: Fragmentation % = how far the heap is from having one big free block.
        // When this exceeds ~60 % SSL handshakes start failing with -32512.
        uint8_t fragPct = (totalFree > 0) ? (uint8_t) (100u - (largest * 100u / totalFree)) : 100u;
        DEBUG_PRINTF("total: %u, allocated: %u, totalFree: %u, minFree: %u, largestFree: %u, frag: %u%%%s",
                     (unsigned) total,
                     (unsigned) info.total_allocated_bytes,
                     (unsigned) totalFree,
                     (unsigned) info.minimum_free_bytes,
                     (unsigned) largest,
                     (unsigned) fragPct,
                     newLine ? "\n" : "");
        s_lastMemoryUsageShownAt = millis();
    }
}
