#include "ShowMemoryUsage.h"

#include "DebugHelper.h"
#include "config_helper.h"
#include <Arduino.h>
#include <ArduinoLog.h>

// initialize static members
static unsigned long s_lastMemoryUsageShownAt = 0;

#ifdef MEMORY_DEBUG_INTERVAL
static const unsigned long interval_ms = MEMORY_DEBUG_INTERVAL;
#else
static const unsigned long interval_ms = 5000; // default is needed for cases of where this method is called without MEMORY_DEBUG_INTERVAL being set
#endif

void ShowMemoryUsage::printSerial(bool force, bool newLine) {
    if (!isDebugEnabled_global())
        return;
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

// ---------------------------------------------------------------------------
// printDetailed — always-on labeled snapshot for instrumentation points.
// Logs every metric that matters for fragmentation analysis:
//   free, largest block, fragmentation %, free block count, total block count,
//   all-time minimum free, and allocated bytes.
// Output prefix [HEAP] makes it easy to grep from serial logs.
// ---------------------------------------------------------------------------
void ShowMemoryUsage::printDetailed(const char *label) {
    if (!isDebugEnabled_global())
        return;
#ifdef MEMORY_DEBUG_INTERVAL
    multi_heap_info_t info;
    heap_caps_get_info(&info, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);

    size_t totalFree = info.total_free_bytes;
    size_t largest = info.largest_free_block;
    size_t minFree = info.minimum_free_bytes;
    size_t allocd = info.total_allocated_bytes;

    // Fragmentation: what percentage of free space is NOT the largest block.
    // 0 % = perfectly unfragmented (one big block).
    // 80 % = heap is mostly free but in many tiny pieces — SSL will fail.
    uint8_t fragPct = (totalFree > 0) ? (uint8_t) (100u - (largest * 100u / totalFree)) : 100u;

    // Block count gives the raw number of fragments.
    // High free_blocks with low largest_free_block = severe fragmentation.
    //
    // ArduinoLog only supports single-char format specifiers (%d, %l, %s, %x, %%).
    // Width modifiers (%-28s, %6u) and %u are NOT supported — they print the format
    // char literally. All size_t values are cast to int (safe: max ESP32 DRAM is 327 KB).
    Log.noticeln("[HEAP] %s | free=%d  largest=%d  frag=%d%%  blk=%d/%d  minFree=%d  alloc=%d",
                 label ? label : "?",
                 (int) totalFree,
                 (int) largest,
                 (int) fragPct,
                 (int) info.free_blocks,
                 (int) info.total_blocks,
                 (int) minFree,
                 (int) allocd);
#endif
}
