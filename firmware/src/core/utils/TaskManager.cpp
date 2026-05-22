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
String TaskManager::activeUrls[TaskManager::MAX_ACTIVE_URLS];
uint8_t TaskManager::activeUrlCount = 0;
static SemaphoreHandle_t taskLimitSemaphore = nullptr;
static StaticTask_t httpWorkerTaskBuffer;
static StackType_t httpWorkerStack[TaskManager::STACK_SIZE];
static TaskHandle_t httpWorkerHandle = nullptr;

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
    if (!taskLimitSemaphore) {
        taskLimitSemaphore = xSemaphoreCreateBinary();
        xSemaphoreGive(taskLimitSemaphore);
    }
}

TaskManager *TaskManager::getInstance() {
    if (!instance) {
        instance = new TaskManager();

        // Spawn the static worker task that handles HTTP requests pinned to Core 0 (PRO_CPU)
        httpWorkerHandle = xTaskCreateStaticPinnedToCore(
            httpWorkerTask,
            "HTTP_WORKER",
            STACK_SIZE,
            nullptr,
            TASK_PRIORITY,
            httpWorkerStack,
            &httpWorkerTaskBuffer,
            0 // Pinned to Core 0
        );
    }
    return instance;
}

void TaskManager::addActiveUrl(const String &url) {
    if (xSemaphoreTake(urlSetMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        if (activeUrlCount < MAX_ACTIVE_URLS) {
            bool found = false;
            for (uint8_t i = 0; i < activeUrlCount; ++i) {
                if (activeUrls[i] == url) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                activeUrls[activeUrlCount++] = url;
            }
        } else {
            Log.errorln("Active URLs list full! Max %d URLs allowed.", MAX_ACTIVE_URLS);
        }
        xSemaphoreGive(urlSetMutex);
    }
}

void TaskManager::removeActiveUrl(const String &url) {
    if (xSemaphoreTake(urlSetMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        for (uint8_t i = 0; i < activeUrlCount; ++i) {
            if (activeUrls[i] == url) {
                for (uint8_t j = i; j < activeUrlCount - 1; ++j) {
                    activeUrls[j] = std::move(activeUrls[j + 1]);
                }
                activeUrls[activeUrlCount - 1] = String();
                --activeUrlCount;
                break;
            }
        }
        xSemaphoreGive(urlSetMutex);
    }
}

bool TaskManager::isUrlActive(const String &url) {
    bool found = false;
    if (xSemaphoreTake(urlSetMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        for (uint8_t i = 0; i < activeUrlCount; ++i) {
            if (activeUrls[i] == url) {
                found = true;
                break;
            }
        }
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

void TaskManager::httpWorkerTask(void *pvParameters) {
    while (true) {
        // Fix 4: Back-pressure — if internal DRAM is low, pause before processing
        // the next request. This gives the heap allocator time to coalesce freed
        // blocks from the previous SSL teardown and FreeType glyph operations,
        // reducing the chance of the next SSL handshake failing with -32512.
        size_t freeHeap = heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        if (freeHeap < 60000) {
            Log.warningln("[HEAP] Low internal DRAM (%d B free), pausing HTTP worker 500 ms", (int) freeHeap);
            vTaskDelay(pdMS_TO_TICKS(500));
        }

        TaskParams *taskParams = nullptr;

        // Block permanently until a task is available in the queue
        if (xQueueReceive(requestQueue, &taskParams, portMAX_DELAY) == pdPASS) {

            if (xSemaphoreTake(taskLimitSemaphore, 0) == pdTRUE) {
                Utils::setBusy(true);
                uint32_t current = activeRequests.fetch_add(1) + 1;

                uint32_t maxSeen = maxConcurrentRequests.load();
                while (current > maxSeen && !maxConcurrentRequests.compare_exchange_weak(maxSeen, current)) {
                    // CAS loop to update max
                }

#ifdef TASKMANAGER_DEBUG
                Log.noticeln("\u2705 Obtained semaphore");
                Log.noticeln("Active requests: %d (Max seen: %d)", activeRequests.load(), maxConcurrentRequests.load());
                Log.noticeln("Processing request: %s (Remaining in queue: %d)",
                             taskParams->url.c_str(),
                             uxQueueMessagesWaiting(requestQueue));
#endif

                CrashTrace::mark("task:create", taskParams->url);
                String url = taskParams->url;
                try {
                    taskParams->taskExec(taskParams->url, taskParams->callback, taskParams->preProcessResponse);
                    delete taskParams;
                    taskParamsCount.fetch_sub(1);
                    removeActiveUrl(url);
                } catch (const std::exception &e) {
                    Log.errorln("Task execution failed with exception: %s", e.what());
                    delete taskParams;
                    taskParamsCount.fetch_sub(1);
                    removeActiveUrl(url);
                } catch (...) {
                    Log.errorln("Task execution failed with unknown exception");
                    delete taskParams;
                    taskParamsCount.fetch_sub(1);
                    removeActiveUrl(url);
                }

                Utils::setBusy(false);
#ifdef TASKMANAGER_DEBUG
                DEBUG_PRINTF("\u2705 Release semaphore\n");
#endif
                xSemaphoreGive(taskLimitSemaphore);
            } else {
                // If we couldn't get the semaphore (shouldn't happen with single worker), put it back
                xQueueSendToFront(requestQueue, &taskParams, 0);
                vTaskDelay(pdMS_TO_TICKS(10));
            }
        }
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
