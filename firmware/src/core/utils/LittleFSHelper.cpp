#include "LittleFSHelper.h"
#include "DebugHelper.h"
#include <ArduinoLog.h>

bool LittleFSHelper::begin() {
    if (!LittleFS.begin()) {
        Log.warningln("LittleFS Mount Failed. Formatting...");
        LittleFS.format();
        if (!LittleFS.begin()) {
            Log.warningln("Failed to mount LittleFS after formatting.");
            return false;
        }
    }
    DEBUG_PRINTF("LittleFS mounted successfully.\n");
    return true;
}

void LittleFSHelper::writeFile(const char *path, const char *message) {
    File file = LittleFS.open(path, "w");
    if (!file) {
        Log.warningln("Failed to open file for writing");
        return;
    }
    file.print(message);
    file.close();
    DEBUG_PRINTF("File written successfully.\n");
}

void LittleFSHelper::readFile(const char *path) {
    File file = LittleFS.open(path, "r");
    if (!file) {
        Log.warningln("Failed to open file for reading");
        return;
    }
    DEBUG_PRINTF("Reading file: %s\n", path);
    while (file.available()) {
        Serial.write(file.read());
    }
    file.close();
}

void LittleFSHelper::deleteFile(const char *path) {
    if (LittleFS.remove(path)) {
        DEBUG_PRINTF("File deleted successfully.\n");
    } else {
        Log.warningln("Failed to delete file.");
    }
}

void LittleFSHelper::listFilesRecursively(const char *dirname) {
    // Ensure the path starts with a `/`
    if (dirname[0] != '/') {
        Log.warningln("Path must start with '/'");
        return;
    }

    File root = LittleFS.open(dirname);
    if (!root || !root.isDirectory()) {
        Log.warningln("Failed to open directory: %s\n", dirname);
        return;
    }

    File file = root.openNextFile();
    while (file) {
        if (file.isDirectory()) {
            // Recursive call for subdirectories
            char fullPath[128];
            snprintf(fullPath, sizeof(fullPath), "%s/%s", dirname, file.name());
            DEBUG_PRINTF("Directory: %s\n", fullPath);
            listFilesRecursively(fullPath);
        } else {
            DEBUG_PRINTF("File: %s/%s, Size: %d\n", dirname, file.name(), file.size());
        }
        file = root.openNextFile();
    }
}
