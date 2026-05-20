#ifndef TASK_FACTORY_H
#define TASK_FACTORY_H

#include "TaskManager.h"
#include <memory>

// Implementation of make_unique for older C++ standards
template <typename T, typename... Args>
std::unique_ptr<T> make_unique(Args &&...args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

class TaskFactory {
public:
    static std::unique_ptr<Task> createHttpGetTask(const String &url, Task::ResponseCallback callback, Task::PreProcessCallback preProcess = nullptr) {
        return make_unique<Task>(
            url, callback, [](const String &u, Task::ResponseCallback cb, Task::PreProcessCallback pp) { TaskFactory::httpGetTask(u, cb, pp); }, preProcess);
    }

    static std::unique_ptr<Task> createMqttTask(const String &topic, Task::ResponseCallback callback) {
        return make_unique<Task>(
            topic, callback, [](const String &t, Task::ResponseCallback cb, Task::PreProcessCallback pp) {
                // Placeholder for MQTT task execution logic
            },
            nullptr);
    }

    // Declare the httpGetTask method
    static void httpGetTask(const String &url, Task::ResponseCallback callback, Task::PreProcessCallback preProcess);
};

#endif // TASK_FACTORY_H
