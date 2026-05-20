#include "GlobalResources.h"
#include "CrashTrace.h"
#include <ArduinoLog.h>

SemaphoreHandle_t dataMutex = nullptr;
EventGroupHandle_t systemEventGroup = nullptr;
TaskHandle_t buttonTaskHandle = nullptr;

void initializeGlobalResources() {
    // Create the mutex
    if (!dataMutex) {
        dataMutex = xSemaphoreCreateMutex();
    }

    // Create the event group
    if (!systemEventGroup) {
        systemEventGroup = xEventGroupCreate();
    }
}

// ── FreeRTOS diagnostic hooks ──────────────────────────────────────────────

/**
 * Called by FreeRTOS when any task overflows its stack.
 * Without this hook the MCU either crashes silently or corrupts memory.
 * The CrashTrace mark ensures the task name survives the subsequent reset.
 */
extern "C" void vApplicationStackOverflowHook(TaskHandle_t /*xTask*/, char *pcTaskName) {
    Log.errorln("STACK OVERFLOW in task: %s", pcTaskName ? pcTaskName : "unknown");
    CrashTrace::mark("STACK_OVERFLOW", pcTaskName ? pcTaskName : "unknown");
    abort(); // triggers a clean panic → readable backtrace via exception_decoder
}

/**
 * Called by FreeRTOS when pvPortMalloc() fails (heap exhausted).
 * Logs the failure and stamps a trace so the reset reason is obvious.
 */
extern "C" void vApplicationMallocFailedHook() {
    Log.errorln("MALLOC FAILED – heap exhausted (free: %u)", ESP.getFreeHeap());
    CrashTrace::mark("MALLOC_FAILED");
    abort();
}
