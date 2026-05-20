#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#include <Arduino.h>
#include <ArduinoLog.h>
#include <atomic>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <functional>
#include <memory>
#include <set>

// Forward declaration of TaskManager to avoid circular dependencies
class TaskManager;

// Define the Task class
class Task {
public:
    using ResponseCallback = std::function<void(int httpCode, const String &response)>;
    using PreProcessCallback = std::function<void(int httpCode, String &response)>;
    using TaskExecCallback = std::function<void()>; // No parameters

    Task(const String &url, ResponseCallback callback, TaskExecCallback taskExec, PreProcessCallback preProcess = nullptr)
        : url(url), callback(callback), preProcessResponse(preProcess), taskExec(taskExec) {}

    String url;
    ResponseCallback callback;
    PreProcessCallback preProcessResponse;
    TaskExecCallback taskExec; // Required

    // Virtual destructor for proper cleanup
    virtual ~Task() = default;
};

class TaskManager {
public:
    using ResponseCallback = Task::ResponseCallback;
    using PreProcessCallback = Task::PreProcessCallback;
    using TaskExecCallback = Task::TaskExecCallback;

    struct TaskParams {
        String url;
        ResponseCallback callback;
        PreProcessCallback preProcessResponse;
        TaskExecCallback taskExec; // Required
    };

    // Make ResponseData public
    struct ResponseData {
        int httpCode;
        String response;
        ResponseCallback callback;
        String url;
    };

    static TaskManager *getInstance();
    bool addTask(std::unique_ptr<Task> task);
    void processTaskResponses();

    // Thread-safe counters (accessed from both main loop and background tasks)
    static std::atomic<uint32_t> activeRequests;
    static std::atomic<uint32_t> maxConcurrentRequests;
    static QueueHandle_t requestQueue;
    static QueueHandle_t responseQueue;

    // Thread-safe URL tracking to prevent duplicate tasks
    static void addActiveUrl(const String &url);
    static void removeActiveUrl(const String &url);
    static bool isUrlActive(const String &url);

    // Add destructor
    ~TaskManager() {
        // Clean up queues
        vQueueDelete(requestQueue);
        vQueueDelete(responseQueue);
    }

    // Add a debug function to check for leaks
    static void checkForLeaks() {
        Log.noticeln("Current TaskParams count: %d", taskParamsCount.load());
        if (taskParamsCount.load() != 0) {
            Log.noticeln("⚠️ Potential memory leak detected!");
        }
    }

private:
    TaskManager();

    static void httpWorkerTask(void *pvParameters);

    static TaskManager *instance;

    static const uint16_t STACK_SIZE = 12000; // Increased from 10000 for safety margin
    static const UBaseType_t TASK_PRIORITY = 1;
    static const UBaseType_t REQUEST_QUEUE_SIZE = 20;
    static const UBaseType_t REQUEST_QUEUE_ITEM_SIZE = sizeof(TaskParams *);
    static const UBaseType_t RESPONSE_QUEUE_SIZE = 20;
    static const UBaseType_t RESPONSE_QUEUE_ITEM_SIZE = sizeof(ResponseData *);
    static const TickType_t QUEUE_CHECK_DELAY = pdMS_TO_TICKS(100); // 100ms between queue checks
    static std::atomic<int> taskParamsCount;

    // Mutex-protected set for tracking active URLs (replaces broken queue rotation)
    static SemaphoreHandle_t urlSetMutex;
    static std::set<String> activeUrls;
};

#endif // TASK_MANAGER_H
