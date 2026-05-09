#include "CrashTrace.h"

#include <ArduinoLog.h>
#include <cstring>
#include <esp_system.h>

struct CrashTraceState {
    uint32_t magic;
    uint32_t sequence;
    uint32_t millisAtMark;
    uint32_t freeHeap;
    char stage[48];
    char detail[96];
};

static constexpr uint32_t CRASH_TRACE_MAGIC = 0x1F0B5A7E;
RTC_NOINIT_ATTR static CrashTraceState s_crashTrace;

void CrashTrace::mark(const char *stage) {
    mark(stage, "");
}

void CrashTrace::mark(const char *stage, const String &detail) {
    if (s_crashTrace.magic != CRASH_TRACE_MAGIC) {
        memset(&s_crashTrace, 0, sizeof(s_crashTrace));
        s_crashTrace.magic = CRASH_TRACE_MAGIC;
    }

    s_crashTrace.sequence++;
    s_crashTrace.millisAtMark = millis();
    s_crashTrace.freeHeap = ESP.getFreeHeap();
    strncpy(s_crashTrace.stage, stage ? stage : "", sizeof(s_crashTrace.stage) - 1);
    s_crashTrace.stage[sizeof(s_crashTrace.stage) - 1] = '\0';
    strncpy(s_crashTrace.detail, detail.c_str(), sizeof(s_crashTrace.detail) - 1);
    s_crashTrace.detail[sizeof(s_crashTrace.detail) - 1] = '\0';
}

void CrashTrace::printLastReset() {
    esp_reset_reason_t reason = esp_reset_reason();
    Log.warningln("Reset reason: %s (%d)", resetReasonToString(reason), reason);

    if (s_crashTrace.magic == CRASH_TRACE_MAGIC) {
        Log.warningln("Last trace: #%u at %ums, heap=%u, stage=%s, detail=%s",
                      s_crashTrace.sequence,
                      s_crashTrace.millisAtMark,
                      s_crashTrace.freeHeap,
                      s_crashTrace.stage,
                      s_crashTrace.detail);
    } else {
        Log.warningln("Last trace: none");
    }
}

const char *CrashTrace::resetReasonToString(esp_reset_reason_t reason) {
    switch (reason) {
    case ESP_RST_POWERON:
        return "power-on";
    case ESP_RST_EXT:
        return "external";
    case ESP_RST_SW:
        return "software";
    case ESP_RST_PANIC:
        return "panic";
    case ESP_RST_INT_WDT:
        return "interrupt-watchdog";
    case ESP_RST_TASK_WDT:
        return "task-watchdog";
    case ESP_RST_WDT:
        return "other-watchdog";
    case ESP_RST_DEEPSLEEP:
        return "deep-sleep";
    case ESP_RST_BROWNOUT:
        return "brownout";
    case ESP_RST_SDIO:
        return "sdio";
    default:
        return "unknown";
    }
}
