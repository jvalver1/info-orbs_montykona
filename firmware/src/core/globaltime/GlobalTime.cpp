#include "GlobalTime.h"

#include "ConfigManager.h"
#include "DebugHelper.h"
#include "Translations.h"
#include "config_helper.h"
#include <ArduinoJson.h>
#include <ArduinoLog.h>

GlobalTime *GlobalTime::m_instance = nullptr;

GlobalTime::GlobalTime() {
    ConfigManager *cm = ConfigManager::getInstance();
    m_timezoneLocation = cm->getConfigString("timezoneLoc", m_timezoneLocation); // config added in MainHelper
    int clockFormat = cm->getConfigInt("clockFormat", CLOCK_FORMAT); // config added in ClockWidget
    m_ntpServer = cm->getConfigString("ntpServer", m_ntpServer); // config added in MainHelper
    DEBUG_PRINTF("GlobalTime initialized, tzLoc=%s, clockFormat=%d, ntpServer=%s\n", m_timezoneLocation.c_str(), clockFormat, m_ntpServer.c_str());
    m_format24hour = (clockFormat == CLOCK_FORMAT_24_HOUR);
    m_timeClient = new NTPClient(m_udp, m_ntpServer.c_str(), 0, m_updateInterval);
    m_timeClient->begin();
}

GlobalTime::~GlobalTime() {
    delete m_timeClient;
}

GlobalTime *GlobalTime::getInstance() {
    if (m_instance == nullptr) {
        m_instance = new GlobalTime();
    }
    return m_instance;
}

time_t GlobalTime::getUnixEpochIfAvailable() {
    return m_instance ? m_instance->getUnixEpoch() : 0;
}

void GlobalTime::updateTime(bool force) {
    if (force || millis() - m_updateTimer > m_oneSecond) {
        m_updateTimer = millis();
        m_timeClient->update();
        if (m_timeClient->isTimeSet()) {
            // NTP time is valid
            // Timezone API calls disabled - using NTP time offset
            // if (m_timeZoneOffset == -1 || (m_nextTimeZoneUpdate > 0 && m_unixEpoch > m_nextTimeZoneUpdate)) {
            //     getTimeZoneOffsetFromAPI();
            // }
            m_unixEpoch = m_timeClient->getEpochTime();
            m_minute = minute(m_unixEpoch);
            if (m_format24hour) {
                m_hour = hour(m_unixEpoch);
            } else {
                m_hour = hourFormat12(m_unixEpoch);
            }
            m_hour24 = hour(m_unixEpoch);
            m_second = second(m_unixEpoch);

            m_day = day(m_unixEpoch);
            m_month = month(m_unixEpoch);
            m_monthName = i18n(t_months, m_month - 1);
            m_year = year(m_unixEpoch);
            m_weekday = i18n(t_weekdays, weekday(m_unixEpoch) - 1);
            m_time = String(m_hour) + ":" + (m_minute < 10 ? "0" + String(m_minute) : String(m_minute));
        }
    }
}

void GlobalTime::getHourAndMinute(int &hour, int &minute) {
    hour = m_hour;
    minute = m_minute;
}

int GlobalTime::getHour() {
    return m_hour;
}

int GlobalTime::getHour24() {
    return m_hour24;
}

String GlobalTime::getHourPadded() {
    if (m_hour < 10) {
        return "0" + String(m_hour);
    } else {
        return String(m_hour);
    }
}

int GlobalTime::getMinute() {
    return m_minute;
}

String GlobalTime::getMinutePadded() {
    if (m_minute < 10) {
        return "0" + String(m_minute);
    } else {
        return String(m_minute);
    }
}

int GlobalTime::getSecond() {
    return m_second;
}

time_t GlobalTime::getUnixEpoch() {
    return m_unixEpoch;
}

int GlobalTime::getDay() {
    return m_day;
}

int GlobalTime::getMonth() {
    return m_month;
}

String GlobalTime::getMonthName() {
    return m_monthName;
}

int GlobalTime::getYear() {
    return m_year;
}

String GlobalTime::getTime() {
    return m_time;
}

String GlobalTime::getWeekday() {
    return m_weekday;
}

String GlobalTime::getDayAndMonth() {
#ifdef WEATHER_UNITS_METRIC
    String retVal = i18n(t_dayMonthFormat);
    retVal.replace("%d", String(m_day));
    retVal.replace("%B", m_monthName);
    return retVal;
#else
    return m_monthName + " " + String(m_day);
#endif
}

#include <HTTPClient.h> // Include the necessary header file

bool GlobalTime::isPM() {
    return hour(m_unixEpoch) >= 12;
}

void GlobalTime::getTimeZoneOffsetFromAPI() {
    // API calls disabled - SSL connection failures, timezone managed by NTP
    DEBUG_PRINTF("Timezone API disabled - using NTP offset\n");
    return;
}

bool GlobalTime::getFormat24Hour() {
    return m_format24hour;
}

bool GlobalTime::setFormat24Hour(bool format24hour) {
    m_format24hour = format24hour;
    return m_format24hour;
}

int GlobalTime::getTimeZoneOffset() {
    return m_timeZoneOffset;
}
