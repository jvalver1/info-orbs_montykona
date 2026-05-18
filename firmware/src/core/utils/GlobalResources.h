#ifndef GLOBAL_RESOURCES_H
#define GLOBAL_RESOURCES_H

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>

// Mutex for protecting shared DTO structures (like Widget Data)
extern SemaphoreHandle_t dataMutex;

// Event group for system states (WiFi, Data available, etc.)
extern EventGroupHandle_t systemEventGroup;

// Task handle for deferred button processing
extern TaskHandle_t buttonTaskHandle;

// Bits for Event Group
#define EVT_WIFI_CONNECTED    (1 << 0)
#define EVT_AP_MODE_ACTIVE    (1 << 1)
#define EVT_WEATHER_AVAILABLE (1 << 2)
#define EVT_STOCK_AVAILABLE   (1 << 3)
#define EVT_NTP_SYNCED        (1 << 4)

void initializeGlobalResources();

#endif // GLOBAL_RESOURCES_H
