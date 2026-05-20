#include "TaskFactory.h"
#include "CrashTrace.h"
#include "DebugHelper.h"
#include "GlobalResources.h"
#include "TaskManager.h"
#include "Utils.h"
#include <ArduinoLog.h>
#include <HTTPClient.h>

#include <lwip/sockets.h>

void TaskFactory::httpGetTask(const String &url, Task::ResponseCallback callback, Task::PreProcessCallback preProcess) {
    CrashTrace::mark("task:http:start", url);
    DEBUG_PRINTF("Starting HTTP request for: %s\n", url.c_str());

    WiFiClient *client = nullptr;
    int httpCode = -1;
    String response;

    auto applyLinger = [&]() {
        if (client != nullptr) {
            int sock_fd = client->fd();
            if (sock_fd >= 0) {
                struct linger sl;
                sl.l_onoff = 1;
                sl.l_linger = 0;
                setsockopt(sock_fd, SOL_SOCKET, SO_LINGER, &sl, sizeof(sl));
            }
        }
    };

    try {
        bool isHttps = url.startsWith("https://");

        if (isHttps) {
            // Fix 3 + Diag 2: Check heap fragmentation BEFORE attempting SSL.
            // mbedTLS requires a contiguous DRAM block of ~36-40 KB for the SSL
            // context, I/O buffers, and certificate chain. If the largest free
            // block is below the threshold, skip this attempt entirely to avoid
            // a failed alloc that leaves partial mbedTLS state on the heap.
            size_t largestBefore = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
            size_t freeBefore = heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
            Log.noticeln("[HEAP] Before SSL alloc: largest=%u B, free=%u B, url=%s",
                         largestBefore, freeBefore, url.c_str());

            // Fix 3: Guard — need at least 40 KB contiguous DRAM for SSL handshake.
            // Note: with CONFIG_MBEDTLS_SSL_IN/OUT_CONTENT_LEN=4096 the requirement
            // drops to ~12-16 KB; keep the guard conservative at 20 KB to be safe.
            static const size_t SSL_MIN_HEAP = 20000;
            if (largestBefore < SSL_MIN_HEAP) {
                Log.errorln("[HEAP] Skipping HTTPS – largest free block only %u B (need %u B). "
                            "Heap too fragmented for SSL handshake.",
                            largestBefore, SSL_MIN_HEAP);
                // Queue a -1 response so the widget callback handles it gracefully
                auto *rd = new (std::nothrow) TaskManager::ResponseData{-1, "", callback, url};
                if (rd)
                    xQueueSend(TaskManager::responseQueue, &rd, 0);
                TaskManager::activeRequests.fetch_sub(1);
                CrashTrace::mark("task:http:skipped-low-heap", url);
                return;
            }

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
                    // Fix 5: Pre-reserve the response String from the Content-Length
                    // header so the String object does not reallocate multiple times
                    // as data arrives, reducing heap fragmentation from String churn.
                    int contentLength = http.getSize(); // returns -1 if unknown
                    if (contentLength > 0) {
                        response.reserve(contentLength + 1);
                    }
                    response = http.getString();
                } else {
                    Log.errorln("HTTP request failed, error code: %d", httpCode);
                }
                applyLinger();
                http.end();

                // Diag 2: Log heap state after SSL teardown so we can track
                // how much DRAM is recovered and how fast fragmentation builds.
                if (url.startsWith("https://")) {
                    size_t largestAfter = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
                    size_t freeAfter = heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
                    Log.noticeln("[HEAP] After SSL cleanup: largest=%u B, free=%u B",
                                 largestAfter, freeAfter);
                }
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

    applyLinger();
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
