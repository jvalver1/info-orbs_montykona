#include "VisualCrossingFeed.h"
#include "../WeatherTranslations.h"
#include "CrashTrace.h"
#include "GlobalTime.h"
#include "TaskFactory.h"
#include "config_helper.h"
#include "DebugHelper.h"

#define WIDGET_PREFIX "[Weather]"

VisualCrossingFeed::VisualCrossingFeed(const String &apiKey, int units)
    : apiKey(apiKey), m_units(units) {}

void VisualCrossingFeed::setupConfig(ConfigManager &config) {

    config.addConfigString("WeatherWidget", "weatherLocation", &m_weatherLocation, 40, t_weatherLocation);
}

bool VisualCrossingFeed::getWeatherData(WeatherDataModel &model) {
    WIDGET_HEAP_SNAP(WIDGET_PREFIX, "pre-fetch");
    String tempUnits = m_units == 0 ? "metric" : "us";

    String lang = I18n::getLanguageString();
    if (lang != "en" && lang != "de" && lang != "fr") {
        // Language is not supported on visualcrossing -> use english
        lang = "en";
    }

    String httpRequestAddress = String(WEATHER_VISUALCROSSING_API_URL) +
                                String(m_weatherLocation.c_str()) + "/next3days?key=" + apiKey + "&unitGroup=" + tempUnits +
                                "&include=days,current&iconSet=icons1&lang=" + lang;

    WeatherDataModel *modelPtr = &model;
    auto task = TaskFactory::createHttpGetTask(
        httpRequestAddress, [this, modelPtr](int httpCode, const String &response) { processResponse(httpCode, response, *modelPtr); });

    if (!task) {
        WIDGET_LOG_ERROR(WIDGET_PREFIX, "Failed to create weather task");
        return false;
    }

    bool success = TaskManager::getInstance()->addTask(std::move(task));
    if (!success) {
        WIDGET_LOG_ERROR(WIDGET_PREFIX, "Failed to add weather task");
    }

    return success;
}

void VisualCrossingFeed::preProcessResponse(int httpCode, String &response) {
    if (httpCode > 0) {
        JsonDocument filter;
        filter["resolvedAddress"] = true;
        filter["currentConditions"]["temp"] = true;
        filter["currentConditions"]["icon"] = true;
        for (int i = 0; i < 4; i++) {
            filter["days"][i]["description"] = true;
            filter["days"][i]["icon"] = true;
            filter["days"][i]["tempmax"] = true;
            filter["days"][i]["tempmin"] = true;
        }

        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, response, DeserializationOption::Filter(filter));

        if (!error) {
            response = doc.as<String>();
        } else {
            // Handle JSON deserialization error
            WIDGET_LOG_ERROR(WIDGET_PREFIX, "Deserialization failed: %s", error.c_str());
        }
    }
}

void VisualCrossingFeed::processResponse(int httpCode, const String &response, WeatherDataModel &model) {
    CrashTrace::mark("weather:visualcrossing:process");
    if (httpCode > 0) {
        JsonDocument filter;
        filter["resolvedAddress"] = true;
        filter["currentConditions"]["temp"] = true;
        filter["currentConditions"]["icon"] = true;
        for (int i = 0; i < 4; i++) {
            filter["days"][i]["description"] = true;
            filter["days"][i]["icon"] = true;
            filter["days"][i]["tempmax"] = true;
            filter["days"][i]["tempmin"] = true;
        }

        WIDGET_HEAP_SNAP(WIDGET_PREFIX, "pre-deserialization");
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, response, DeserializationOption::Filter(filter));
        WIDGET_HEAP_SNAP(WIDGET_PREFIX, "post-deserialization");

        if (!error) {
            model.setCityName(doc["resolvedAddress"].as<String>());
            model.setCurrentTemperature(doc["currentConditions"]["temp"].as<float>());
            model.setCurrentText(doc["days"][0]["description"].as<String>());

            model.setCurrentIcon(doc["currentConditions"]["icon"].as<String>());
            model.setTodayHigh(doc["days"][0]["tempmax"].as<float>());
            model.setTodayLow(doc["days"][0]["tempmin"].as<float>());
            for (int i = 0; i < 3; i++) {
                model.setDayIcon(i, doc["days"][i + 1]["icon"].as<String>());
                model.setDayHigh(i, doc["days"][i + 1]["tempmax"].as<float>());
                model.setDayLow(i, doc["days"][i + 1]["tempmin"].as<float>());
            }
        } else {
            // Handle JSON deserialization error
            WIDGET_LOG_ERROR(WIDGET_PREFIX, "Deserialization failed: %s", error.c_str());
        }
    } else {
        WIDGET_LOG_ERROR(WIDGET_PREFIX, "HTTP request failed, error code: %d", httpCode);
    }
}
