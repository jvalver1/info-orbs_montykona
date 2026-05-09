#ifndef CRASH_TRACE_H
#define CRASH_TRACE_H

#include <Arduino.h>
#include <esp_system.h>

class CrashTrace {
public:
    static void mark(const char *stage);
    static void mark(const char *stage, const String &detail);
    static void printLastReset();

private:
    static const char *resetReasonToString(esp_reset_reason_t reason);
};

#endif // CRASH_TRACE_H
