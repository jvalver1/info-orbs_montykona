#include "TaskFactory.h"
#include "CrashTrace.h"
#include "DebugHelper.h"
#include "GlobalResources.h"
#include "TaskManager.h"
#include "Utils.h"
#include <ArduinoLog.h>
#include <HTTPClient.h>

void TaskFactory::httpGetTask(const String &url, Task::ResponseCallback callback, Task::PreProcessCallback preProcess) {
    CrashTrace::mark("task:http:start", url);
    DEBUG_PRINTF("Starting HTTP request for: %s\n", url.c_str());

    WiFiClient *client = nullptr;
    int httpCode = -1;
    String response;

    // Keep the client alive until HTTPClient has been destroyed. HTTPClient
    // retains a pointer to external clients and may touch it in its destructor.
    {
        HTTPClient http;
        bool isHttps = url.startsWith("https://");

        if (isHttps) {
            client = new WiFiClientSecure();
            static_cast<WiFiClientSecure *>(client)->setInsecure();
            if (!http.begin(*client, url)) {
                Log.errorln("HTTP begin failed for %s", url.c_str());
            }
        } else {
            client = new WiFiClient();
            if (!http.begin(*client, url)) {
                Log.errorln("HTTP begin failed for %s", url.c_str());
            }
        }

        http.setTimeout(10000);
        httpCode = http.GET();

        if (httpCode > 0) {
            response = http.getString();
        } else {
            Log.errorln("HTTP request failed, error code: %d", httpCode);
        }

        http.end();
    }

    delete client;

    if (preProcess) {
        CrashTrace::mark("task:http:preprocess", url);
        preProcess(httpCode, response);
    }

    CrashTrace::mark("task:http:queue-response", url);
    auto *responseData = new TaskManager::ResponseData{httpCode, response, callback, url};

    if (xQueueSend(TaskManager::responseQueue, &responseData, 0) != pdPASS) {
        Log.errorln("Failed to queue response");
        delete responseData;
    }

    TaskManager::activeRequests.fetch_sub(1);
    CrashTrace::mark("task:http:done", url);

#ifdef TASKMANAGER_DEBUG
    Log.noticeln("Active requests now: %d", TaskManager::activeRequests);
    UBaseType_t highWater = uxTaskGetStackHighWaterMark(NULL);
    Log.noticeln("Remaining task stack space: %d", highWater);
#endif
}
