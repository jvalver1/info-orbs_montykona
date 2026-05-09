#include "TempestFeed.h"
#include "../WeatherTranslations.h"
#include "CrashTrace.h"
#include "GlobalTime.h"
#include "TaskFactory.h"
#include "config_helper.h"

TempestFeed::TempestFeed(const String &apiKey, int units)
    : apiKey(apiKey), m_units(units) {}

void TempestFeed::setupConfig(ConfigManager &config) {
    // Define the configuration for stationId and stationName
    config.addConfigString("WeatherWidget", "tempestStatId", &m_stationId, 10, t_tempestStationId);
    config.addConfigString("WeatherWidget", "tempestStatName", &m_stationName, 15, t_tempestStationName);
}

bool TempestFeed::getWeatherData(WeatherDataModel &model) {

    Log.traceln("TempestFeed: Fetching weather data for stationId=%s, stationName=%s", m_stationId.c_str(), m_stationName.c_str());

    String lang = I18n::getLanguageString();

    String tempUnits = m_units == 0 ? "c" : "f";

    String httpRequestAddress = String(m_proxyUrl.c_str()) + "?station_id=" + String(m_stationId.c_str()) +
                                "&units_temp=" + tempUnits + "&units_wind=mph&units_pressure=mb&units_precip=in&units_distance=mi&api_key=" + apiKey;

    WeatherDataModel *modelPtr = &model;
    auto task = TaskFactory::createHttpGetTask(
        httpRequestAddress, [this, modelPtr](int httpCode, const String &response) { processResponse(httpCode, response, *modelPtr); }, [this](int httpCode, String &response) { preProcessResponse(httpCode, response); });

    if (!task) {
        Log.errorln("Failed to create weather task");
        return false;
    }

    bool success = TaskManager::getInstance()->addTask(std::move(task));
    if (!success) {
        Log.errorln("Failed to add weather task");
    }

    return success;
}

void TempestFeed::preProcessResponse(int httpCode, String &response) {
    if (httpCode > 0) {
        JsonDocument filter;
        filter["current_conditions"]["air_temperature"] = true;
        filter["current_conditions"]["icon"] = true;
        for (int i = 0; i < 3; i++) {
            filter["forecast"]["daily"][i]["air_temp_high"] = true;
            filter["forecast"]["daily"][i]["air_temp_low"] = true;
            filter["forecast"]["daily"][i]["conditions"] = true;
            filter["forecast"]["daily"][i]["icon"] = true;
        }

        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, response, DeserializationOption::Filter(filter));

        if (!error) {
            response = doc.as<String>();
        } else {
            // Handle JSON deserialization error
            Log.errorln("Deserialization failed: %s", error.c_str());
        }
    }
}

void TempestFeed::processResponse(int httpCode, const String &response, WeatherDataModel &model) {
    CrashTrace::mark("weather:tempest:process");
    if (httpCode > 0) {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, response);

        if (!error) {
            model.setCityName(String(m_stationName.c_str())); // Convert std::string to String
            model.setCurrentTemperature(doc["current_conditions"]["air_temperature"].as<float>());
            model.setCurrentText(doc["forecast"]["daily"][0]["conditions"].as<String>());

            model.setCurrentIcon(translateIcon(doc["current_conditions"]["icon"]));
            model.setTodayHigh(doc["forecast"]["daily"][0]["air_temp_high"].as<float>());
            model.setTodayLow(doc["forecast"]["daily"][0]["air_temp_low"].as<float>());
            for (int i = 0; i < 3; i++) {
                model.setDayIcon(i, translateIcon(doc["forecast"]["daily"][i]["icon"]));
                model.setDayHigh(i, doc["forecast"]["daily"][i]["air_temp_high"].as<float>());
                model.setDayLow(i, doc["forecast"]["daily"][i]["air_temp_low"].as<float>());
            }
        } else {
            // Handle JSON deserialization error
            Log.errorln("Deserialization failed: %s", error.c_str());
        }
    } else {
        Log.errorln("HTTP request failed, error code: %d\n", httpCode);
    }
}

String TempestFeed::translateIcon(const std::string &icon) {
    // Define the mapping of input strings to simplified weather icons
    static const std::unordered_map<std::string, std::string> iconMapping = {
        {"clear-day", "clear-day"},
        {"clear-night", "clear-night"},
        {"partly-cloudy-day", "partly-cloudy-day"},
        {"partly-cloudy-night", "partly-cloudy-night"},
        {"cloudy", "partly-cloudy-day"}, // Map cloudy to partly-cloudy-day
        {"rain", "rain"},
        {"rainy", "rain"},
        {"sleet", "rain"},
        {"possibly-rain-day", "rain"},
        {"possibly-rain-night", "rain"},
        {"possibly-sleet-day", "rain"},
        {"possibly-sleet-night", "rain"},
        {"snow", "snow"},
        {"possibly-snow-day", "snow"},
        {"possibly-snow-night", "snow"},
        {"fog", "fog"},
        {"foggy", "fog"},
        {"hail", "rain"}, // Example: treat hail as rain
        {"thunderstorm", "rain"}, // Example: treat thunderstorm as rain
        {"possibly-thunderstorm-day", "rain"}, // Example: treat thunderstorm as rain
        {"possibly-thunderstorm-night", "rain"}, // Example: treat thunderstorm as rain
        {"wind", "clear-day"}, // Example: map windy to clear-day
        {"windy", "clear-day"}, // Example: map windy to clear-day
        {"tornado", "rain"} // Example: treat tornado as rain
    };

    // Find the icon in the mapping and return the corresponding simplified value
    auto it = iconMapping.find(icon);
    if (it != iconMapping.end()) {
        return String(it->second.c_str());
    }
    return "clear-day";
}
