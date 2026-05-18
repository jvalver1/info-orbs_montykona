#include "GlobalResources.h"

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
