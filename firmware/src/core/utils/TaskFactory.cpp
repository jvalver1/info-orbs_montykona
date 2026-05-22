#include "TaskFactory.h"
#include "CrashTrace.h"
#include "DebugHelper.h"
#include "GlobalResources.h"
#include "ShowMemoryUsage.h"
#include "TaskManager.h"
#include "Utils.h"
#include <ArduinoLog.h>
#include <HTTPClient.h>
#include <utility>
#include <WiFiClient.h>
#include <ESP_SSLClient.h>

#include <lwip/sockets.h>

class ESP_SSLClientWrapper : public WiFiClient {
private:
    WiFiClient m_baseClient;
    ESP_SSLClient m_sslClient;
    int m_lastSyncedError = 0;

    void syncWriteError() {
        int sslErr = m_sslClient.getWriteError();
        int wrapperErr = getWriteError();
        if (wrapperErr == 0 && m_lastSyncedError != 0) {
            m_sslClient.clearWriteError();
            m_lastSyncedError = 0;
        } else if (sslErr != 0) {
            setWriteError(sslErr);
            m_lastSyncedError = sslErr;
        } else if (sslErr == 0 && wrapperErr != 0) {
            setWriteError(0);
            m_lastSyncedError = 0;
        }
    }

public:
    ESP_SSLClientWrapper() : m_lastSyncedError(0) {
        m_sslClient.setClient(&m_baseClient);
        m_sslClient.setBufferSizes(5120, 1024);
        m_sslClient.setInsecure();
        m_sslClient.setDebugLevel(isDebugEnabled_global() && LOG_LEVEL >= LOG_LEVEL_INFO ? esp_ssl_debug_info : esp_ssl_debug_none);
    }

    int connect(IPAddress ip, uint16_t port) override {
        syncWriteError();
        int ret = m_sslClient.connect(ip, port);
        syncWriteError();
        return ret;
    }
    int connect(const char *host, uint16_t port) override {
        syncWriteError();
        int ret = m_sslClient.connect(host, port);
        syncWriteError();
        return ret;
    }
    int connect(IPAddress ip, uint16_t port, int32_t timeout) override {
        syncWriteError();
        int ret = m_sslClient.connect(ip, port, timeout);
        syncWriteError();
        return ret;
    }
    int connect(const char *host, uint16_t port, int32_t timeout) override {
        syncWriteError();
        int ret = m_sslClient.connect(host, port, timeout);
        syncWriteError();
        return ret;
    }
    size_t write(uint8_t b) override {
        syncWriteError();
        size_t ret = m_sslClient.write(b);
        syncWriteError();
        return ret;
    }
    size_t write(const uint8_t *buf, size_t size) override {
        syncWriteError();
        size_t ret = m_sslClient.write(buf, size);
        syncWriteError();
        return ret;
    }
    int available() override {
        syncWriteError();
        int ret = m_sslClient.available();
        syncWriteError();
        return ret;
    }
    int read() override {
        syncWriteError();
        int ret = m_sslClient.read();
        syncWriteError();
        return ret;
    }
    int read(uint8_t *buf, size_t size) override {
        syncWriteError();
        int ret = m_sslClient.read(buf, size);
        syncWriteError();
        return ret;
    }
    int peek() override {
        syncWriteError();
        int ret = m_sslClient.peek();
        syncWriteError();
        return ret;
    }
    void flush() override {
        syncWriteError();
        m_sslClient.flush();
        syncWriteError();
    }
    void stop() override {
        syncWriteError();
        m_sslClient.stop();
        m_baseClient.stop();
        syncWriteError();
    }
    uint8_t connected() override {
        syncWriteError();
        uint8_t ret = m_sslClient.connected();
        syncWriteError();
        return ret;
    }
    operator bool() override {
        syncWriteError();
        bool ret = (bool)m_sslClient;
        syncWriteError();
        return ret;
    }
    int setTimeout(uint32_t seconds) override {
        syncWriteError();
        m_baseClient.setTimeout(seconds);
        int ret = m_sslClient.setTimeout(seconds);
        syncWriteError();
        return ret;
    }
    int getLastSSLError(char *dest = nullptr, size_t len = 0) {
        return m_sslClient.getLastSSLError(dest, len);
    }
    int fd() const {
        return const_cast<WiFiClient&>(m_baseClient).fd();
    }
};

void TaskFactory::httpGetTask(const String &url, Task::ResponseCallback callback, Task::PreProcessCallback preProcess) {
    CrashTrace::mark("task:http:start", url);
    DEBUG_PRINTF("Starting HTTP request for: %s\n", url.c_str());

    WiFiClient *baseClient = nullptr;
    ESP_SSLClientWrapper *sslClient = nullptr;
    WiFiClient *client = nullptr;
    int httpCode = -1;
    String response;
    bool isHttps = url.startsWith("https://");

    auto applyLinger = [&]() {
        if (client != nullptr) {
            int sock_fd = -1;
            if (isHttps && sslClient != nullptr) {
                sock_fd = sslClient->fd();
            } else if (baseClient != nullptr) {
                sock_fd = baseClient->fd();
            }
            if (sock_fd >= 0) {
                struct linger sl;
                sl.l_onoff = 1;
                sl.l_linger = 0;
                setsockopt(sock_fd, SOL_SOCKET, SO_LINGER, &sl, sizeof(sl));
            }
        }
    };

    try {
        if (isHttps) {
            // Check heap fragmentation BEFORE attempting SSL.
            // BearSSL wrapped client uses ~6-7 KB buffer space + ~10 KB context overhead.
            // A largest contiguous free block of 15 KB is comfortable.
            size_t largestBefore = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
            size_t freeBefore = heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
            Log.noticeln("[HEAP] Before SSL alloc: largest=%d B, free=%d B, url=%s",
                         (int)largestBefore, (int)freeBefore, url.c_str());

            static const size_t SSL_MIN_HEAP = 15000;
            if (largestBefore < SSL_MIN_HEAP) {
                Log.errorln("[HEAP] Skipping HTTPS – largest free block only %d B (need %d B). "
                            "Heap too fragmented for SSL handshake.",
                            (int)largestBefore, (int)SSL_MIN_HEAP);
                // Queue a -1 response so the widget callback handles it gracefully
                auto *rd = new (std::nothrow) TaskManager::ResponseData{-1, "", callback, url};
                if (rd)
                    xQueueSend(TaskManager::responseQueue, &rd, 0);
                TaskManager::activeRequests.fetch_sub(1);
                CrashTrace::mark("task:http:skipped-low-heap", url);
                return;
            }

            HEAP_SNAP("ssl:pre-new-client");

            sslClient = new (std::nothrow) ESP_SSLClientWrapper();
            if (sslClient == nullptr) {
                Log.errorln("Failed to allocate ESP_SSLClientWrapper due to heap pressure");
            } else {
                client = sslClient;
            }

            HEAP_SNAP("ssl:post-new-client");

        } else {
            baseClient = new (std::nothrow) WiFiClient();
            if (baseClient == nullptr) {
                Log.errorln("Failed to allocate WiFiClient due to heap pressure");
            } else {
                client = baseClient;
            }
        }

        if (client != nullptr) {
            HTTPClient http;
            if (http.begin(*client, url)) {
                http.setTimeout(10000);

                // Phase 1 Diag: snapshot before http.GET() — the handshake + TLS
                // record exchange happens inside GET(), consuming significant DRAM.
                HEAP_SNAP("http:pre-GET");

                httpCode = http.GET();

                // Phase 1 Diag: snapshot after GET() but before reading the body.
                // This reveals the peak SSL + receive-buffer cost during the handshake.
                HEAP_SNAP("http:post-GET");

                if (httpCode > 0) {
                    // Fix 5: Pre-reserve the response String from the Content-Length
                    // header so the String object does not reallocate multiple times
                    // as data arrives, reducing heap fragmentation from String churn.
                    int contentLength = http.getSize(); // returns -1 if unknown
                    if (contentLength > 0) {
                        response.reserve(contentLength + 1);
                    }

                    // Phase 1 Diag: snapshot before getString() — the response body
                    // is buffered here, potentially allocating 2-10 KB for JSON payload.
                    HEAP_SNAP("http:pre-getString");

                    response = http.getString();

                    // Phase 1 Diag: snapshot after getString(). The difference from
                    // pre-getString tells us the actual response body allocation cost.
                    char snapLabel[48];
                    snprintf(snapLabel, sizeof(snapLabel), "http:post-getString(%d B)", (int)response.length());
                    HEAP_SNAP(snapLabel);

                } else {
                    if (isHttps && sslClient != nullptr) {
                        char sslErrBuf[128];
                        int sslErrCode = sslClient->getLastSSLError(sslErrBuf, sizeof(sslErrBuf));
                        Log.errorln("HTTP request failed, error code: %d. SSL Error: %d - %s", httpCode, sslErrCode, sslErrBuf);
                    } else {
                        Log.errorln("HTTP request failed, error code: %d", httpCode);
                    }
                }
                applyLinger();
                http.end();

                // Diag 2: Log heap state after SSL teardown so we can track
                // how much DRAM is recovered and how fast fragmentation builds.
                if (url.startsWith("https://")) {
                    size_t largestAfter = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
                    size_t freeAfter = heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
                    Log.noticeln("[HEAP] After SSL cleanup: largest=%d B, free=%d B",
                                 (int)largestAfter, (int)freeAfter);
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

    // Phase 1 Diag: snapshot before delete client.
    HEAP_SNAP("http:pre-delete-client");
    delete sslClient;
    delete baseClient;
    client = nullptr;
    baseClient = nullptr;
    sslClient = nullptr;
    HEAP_SNAP("http:post-delete-client");

    if (isHttps) {
        // Phase 1 Diag: snapshot after the post-SSL coalescing delay.
        // A good result: largest free block should recover close to its pre-SSL value.
        // A bad result: it stays fragmented — that is the key diagnostic signal.
        vTaskDelay(pdMS_TO_TICKS(1500));
        HEAP_SNAP("http:post-ssl-delay(1500ms)");
    }

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
    auto *responseData = new (std::nothrow) TaskManager::ResponseData{httpCode, std::move(response), callback, url};

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
    Log.noticeln("Active requests now: %d", TaskManager::activeRequests.load());
    UBaseType_t highWater = uxTaskGetStackHighWaterMark(NULL);
    Log.noticeln("Remaining task stack space: %d", highWater);
#endif
}
