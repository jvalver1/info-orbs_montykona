#include "TaskManager.h"
#include "CrashTrace.h"
#include "DebugHelper.h"
#include "GlobalResources.h"
#include "Utils.h"
#include <ArduinoLog.h>
#include <HTTPClient.h>
#include <memory>

TaskManager *TaskManager::instance = nullptr;
QueueHandle_t TaskManager::requestQueue = nullptr;
QueueHandle_t TaskManager::responseQueue = nullptr;
std::atomic<uint32_t> TaskManager::activeRequests{0};
std::atomic<uint32_t> TaskManager::maxConcurrentRequests{0};
std::atomic<int> TaskManager::taskParamsCount{0};
SemaphoreHandle_t TaskManager::urlSetMutex = nullptr;
std::set<String> TaskManager::activeUrls;

TaskManager::TaskManager() {
    if (!requestQueue) {
        requestQueue = xQueueCreate(REQUEST_QUEUE_SIZE, REQUEST_QUEUE_ITEM_SIZE);
    }
    if (!responseQueue) {
        responseQueue = xQueueCreate(RESPONSE_QUEUE_SIZE, RESPONSE_QUEUE_ITEM_SIZE);
    }
    if (!urlSetMutex) {
        urlSetMutex = xSemaphoreCreateMutex();
    }
}

TaskManager *TaskManager::getInstance() {
    if (!instance) {
        instance = new TaskManager();
    }
    return instance;
}

void TaskManager::addActiveUrl(const String &url) {
    if (xSemaphoreTake(urlSetMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        activeUrls.insert(url);
        xSemaphoreGive(urlSetMutex);
    }
}

void TaskManager::removeActiveUrl(const String &url) {
    if (xSemaphoreTake(urlSetMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        activeUrls.erase(url);
        xSemaphoreGive(urlSetMutex);
    }
}

bool TaskManager::isUrlActive(const String &url) {
    bool found = false;
    if (xSemaphoreTake(urlSetMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        found = activeUrls.count(url) > 0;
        xSemaphoreGive(urlSetMutex);
    }
    return found;
}

bool TaskManager::addTask(std::unique_ptr<Task> task) {
    if (isUrlActive(task->url)) {
        Log.errorln("Duplicate Task. Task already in the queue to waiting to be processed.");
        return false;
    }

    auto *params = new TaskParams{task->url, task->callback, task->preProcessResponse, task->taskExec};
    taskParamsCount.fetch_add(1);
#ifdef TASKMANAGER_DEBUG
    Log.noticeln("TaskParams created: %d", taskParamsCount.load());
#endif

    if (xQueueSend(requestQueue, &params, 0) != pdPASS) {
        delete params;
        taskParamsCount.fetch_sub(1);
#ifdef TASKMANAGER_DEBUG
        Log.noticeln("TaskParams deleted (queue full): %d", taskParamsCount.load());
#endif
        return false;
    }

    // Track URL as active so duplicates are rejected
    addActiveUrl(task->url);

    return true;
}

void TaskManager::processAwaitingTasks() {
    // First check if there are any requests to process
    if (uxQueueMessagesWaiting(requestQueue) == 0) {
        return; // No requests in queue
    }

    if (xSemaphoreTake(taskSemaphore, 0) != pdTRUE) {
        return;
    }

    Utils::setBusy(true);
    uint32_t current = activeRequests.fetch_add(1) + 1;

    uint32_t maxSeen = maxConcurrentRequests.load();
    while (current > maxSeen && !maxConcurrentRequests.compare_exchange_weak(maxSeen, current)) {
        // CAS loop to update max
    }
#ifdef TASKMANAGER_DEBUG
    Log.noticeln("\u2705 Obtained semaphore");
    Log.noticeln("Active requests: %d (Max seen: %d)", activeRequests.load(), maxConcurrentRequests.load());
#endif

    // Get next request
    TaskParams *taskParams = nullptr;
    if (xQueueReceive(requestQueue, &taskParams, 0) != pdPASS) {
        DEBUG_PRINTF("\u26a0\ufe0f Queue empty after size check!\n");
        activeRequests.fetch_sub(1);
        Utils::setBusy(false);
        xSemaphoreGive(taskSemaphore);
        return;
    }

#ifdef TASKMANAGER_DEBUG
    Log.noticeln("Processing request: %s (Remaining in queue: %d)",
                 taskParams->url.c_str(),
                 uxQueueMessagesWaiting(requestQueue));
#endif

    CrashTrace::mark("task:create", taskParams->url);
    TaskHandle_t taskHandle;
    BaseType_t result = xTaskCreate(
        [](void *params) {
            auto *taskParams = static_cast<TaskParams *>(params);
            String url = taskParams->url; // Copy URL before taskParams is deleted
            taskParams->taskExec();
            delete taskParams; // Ensure cleanup after execution
            taskParamsCount.fetch_sub(1);

            // Remove URL from active set now that task is complete
            removeActiveUrl(url);

            Utils::setBusy(false);
            DEBUG_PRINTF("\u2705 Release semaphore\n");
            xSemaphoreGive(taskSemaphore);
            vTaskDelete(nullptr);
        },
        "TASK_EXEC",
        STACK_SIZE,
        taskParams,
        TASK_PRIORITY,
        &taskHandle);

    if (result != pdPASS) {
        Log.errorln("Failed to create HTTP request task");
        String url = taskParams->url;
        delete taskParams; // Ensure the object is deleted if task creation fails
        taskParamsCount.fetch_sub(1);
        Log.errorln("TaskParams deleted (task creation failed): %d", taskParamsCount.load());
        removeActiveUrl(url);
        Utils::setBusy(false);
        xSemaphoreGive(taskSemaphore);
    }
}

void TaskManager::processTaskResponses() {

#ifdef TASKMANAGER_DEBUG
    // Periodically check for TaskParams leaks
    static unsigned long lastLeakCheck = 0;
    if (millis() - lastLeakCheck > 30000) { // Check every 30 seconds
        TaskManager::checkForLeaks();
        lastLeakCheck = millis();
    }
#endif

    if (uxQueueMessagesWaiting(responseQueue) == 0) {
        return;
    }

    ResponseData *responseData;
    while (xQueueReceive(responseQueue, &responseData, 0) == pdPASS) {
        CrashTrace::mark("task:response-callback", responseData->url);
        responseData->callback(responseData->httpCode, responseData->response);
        delete responseData; // Ensure the object is deleted after processing
    }
}
