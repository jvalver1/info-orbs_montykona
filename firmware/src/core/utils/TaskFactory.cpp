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

    try {
        bool isHttps = url.startsWith("https://");

        if (isHttps) {
            client = new (std::nothrow) WiFiClientSecure();
            if (client == nullptr) {
                Log.errorln("Failed to allocate WiFiClientSecure due to heap pressure");
            } else {
                static_cast<WiFiClientSecure *>(client)->setInsecure();
            }
        } else {
            client = new (std::nothrow) WiFiClient();
            if (client == nullptr) {
                Log.errorln("Failed to allocate WiFiClient due to heap pressure");
            }
        }

        if (client != nullptr) {
            HTTPClient http;
            if (http.begin(*client, url)) {
                http.setTimeout(10000);
                httpCode = http.GET();

                if (httpCode > 0) {
                    response = http.getString();
                } else {
                    Log.errorln("HTTP request failed, error code: %d", httpCode);
                }
                http.end();
            } else {
                Log.errorln("HTTP begin failed for %s", url.c_str());
            }
        }
    } catch (const std::exception &e) {
        Log.errorln("Exception during HTTP execution: %s", e.what());
        httpCode = -1;
    } catch (...) {
        Log.errorln("Unknown exception during HTTP execution");
        httpCode = -1;
    }

    delete client;

    if (preProcess) {
        CrashTrace::mark("task:http:preprocess", url);
        try {
            preProcess(httpCode, response);
        } catch (const std::exception &e) {
            Log.errorln("Exception in task preProcess: %s", e.what());
        } catch (...) {
            Log.errorln("Unknown exception in task preProcess");
        }
    }

    CrashTrace::mark("task:http:queue-response", url);
    auto *responseData = new (std::nothrow) TaskManager::ResponseData{httpCode, response, callback, url};

    if (responseData == nullptr) {
        Log.errorln("Failed to allocate ResponseData due to heap pressure");
    } else {
        if (xQueueSend(TaskManager::responseQueue, &responseData, 0) != pdPASS) {
            Log.errorln("Failed to queue response");
            delete responseData;
        }
    }

    TaskManager::activeRequests.fetch_sub(1);
    CrashTrace::mark("task:http:done", url);

#ifdef TASKMANAGER_DEBUG
    Log.noticeln("Active requests now: %d", TaskManager::activeRequests);
    UBaseType_t highWater = uxTaskGetStackHighWaterMark(NULL);
    Log.noticeln("Remaining task stack space: %d", highWater);
#endif
}
