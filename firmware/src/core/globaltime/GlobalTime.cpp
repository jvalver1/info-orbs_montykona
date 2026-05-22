#include "GlobalTime.h"

#include "ConfigManager.h"
#include "DebugHelper.h"
#include "PosixTimezones.h"
#include "Translations.h"
#include "config_helper.h"
#include <ArduinoJson.h>
#include <ArduinoLog.h>
#include <freertos/semphr.h>

GlobalTime *GlobalTime::m_instance = nullptr;
static SemaphoreHandle_t s_tzMutex = nullptr;

GlobalTime::GlobalTime() {
    ConfigManager *cm = ConfigManager::getInstance();
    m_timezoneLocation = cm->getConfigString("timezoneLoc", m_timezoneLocation); // config added in MainHelper
    int clockFormat = cm->getConfigInt("clockFormat", CLOCK_FORMAT); // config added in ClockWidget
    m_ntpServer = cm->getConfigString("ntpServer", m_ntpServer); // config added in MainHelper
    DEBUG_PRINTF("GlobalTime initialized, tzLoc=%s, clockFormat=%d, ntpServer=%s\n", m_timezoneLocation.c_str(), clockFormat, m_ntpServer.c_str());
    m_format24hour = (clockFormat == CLOCK_FORMAT_24_HOUR);
    m_timeClient = new NTPClient(m_udp, m_ntpServer.c_str(), 0, m_updateInterval);
    m_timeClient->begin();

    // Create TZ mutex for thread-safe timezone manipulations
    // Use recursive mutex because getOffsetForTimezone calls calculateActiveTimezoneOffset
    if (!s_tzMutex) {
        s_tzMutex = xSemaphoreCreateRecursiveMutex();
    }

    // Configure ESP32 system time with POSIX timezone for automatic DST
    const char *posixTz = getPosixTz(m_timezoneLocation.c_str());
    if (posixTz != nullptr) {
        DEBUG_PRINTF("GlobalTime: Using POSIX TZ for '%s': %s\n", m_timezoneLocation.c_str(), posixTz);
        configTzTime(posixTz, m_ntpServer.c_str(), "time.nist.gov");
        m_posixTzConfigured = true;
    } else {
        DEBUG_PRINTF("GlobalTime: No POSIX TZ found for '%s', falling back to UTC\n", m_timezoneLocation.c_str());
        configTzTime("UTC0", m_ntpServer.c_str(), "time.nist.gov");
        m_posixTzConfigured = true;
    }
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
        time_t now;
        time(&now);
        if ((m_posixTzConfigured && now > 1735689600) || m_timeClient->isTimeSet()) {
            // Use ESP32 system time with POSIX timezone (handles DST automatically)
            struct tm timeinfo;
            if (m_posixTzConfigured && now > 1735689600) {
                localtime_r(&now, &timeinfo);
                // getLocalTime succeeded — use system time with DST applied
                m_unixEpoch = now;
                m_minute = timeinfo.tm_min;
                if (m_format24hour) {
                    m_hour = timeinfo.tm_hour;
                } else {
                    int h = timeinfo.tm_hour % 12;
                    m_hour = (h == 0) ? 12 : h;
                }
                m_hour24 = timeinfo.tm_hour;
                m_second = timeinfo.tm_sec;
                m_day = timeinfo.tm_mday;
                m_month = timeinfo.tm_mon + 1; // tm_mon is 0-based
                m_monthName = i18n(t_months, m_month - 1);
                m_year = timeinfo.tm_year + 1900;
                m_weekday = i18n(t_weekdays, timeinfo.tm_wday); // tm_wday: 0=Sunday
                m_time = String(m_hour) + ":" + (m_minute < 10 ? "0" + String(m_minute) : String(m_minute));

                m_timeZoneOffset = calculateActiveTimezoneOffset(now);
                if (!m_timeZoneFetched) {
                    DEBUG_PRINTF("GlobalTime: Local time via POSIX TZ: %04d-%02d-%02d %02d:%02d:%02d (UTC offset: %d sec, DST: %s)\n",
                                 m_year, m_month, m_day, m_hour24, m_minute, m_second,
                                 m_timeZoneOffset, timeinfo.tm_isdst > 0 ? "active" : "inactive");
                    m_timeZoneFetched = true;
                }
            } else {
                // Fallback: use NTPClient raw epoch (UTC)
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
    return m_hour24 >= 12;
}

void GlobalTime::getTimeZoneOffsetFromAPI() {
    // No longer needed — POSIX timezone handles DST automatically
    DEBUG_PRINTF("GlobalTime: Timezone managed by POSIX TZ rules (no API needed)\n");
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

bool GlobalTime::isTimeValid() {
    return m_unixEpoch > 1735689600; // 2025-01-01 UTC
}

static time_t tmToSeconds(const struct tm *tm) {
    int year = tm->tm_year + 1900;
    int month = tm->tm_mon; // 0-11
    int day = tm->tm_mday - 1; // 0-based

    static const int days_before_month[] = {
        0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};

    int y = year - 1970;
    int leap_days = (y + 1) / 4 - (y + 69) / 100 + (y + 369) / 400;

    time_t days = y * 365LL + leap_days + days_before_month[month] + day;

    bool is_leap = (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
    if (is_leap && month > 1) {
        days += 1;
    }

    return days * 86400LL + tm->tm_hour * 3600LL + tm->tm_min * 60LL + tm->tm_sec;
}

int GlobalTime::calculateActiveTimezoneOffset(time_t utcEpoch) {
    // Protect global TZ environment variable from concurrent access by background tasks
    if (s_tzMutex && xSemaphoreTakeRecursive(s_tzMutex, pdMS_TO_TICKS(500)) != pdTRUE) {
        Log.warningln("Failed to acquire TZ mutex in calculateActiveTimezoneOffset");
        return 0;
    }

    struct tm tmLocal;
    localtime_r(&utcEpoch, &tmLocal);
    int offset = (int) (tmToSeconds(&tmLocal) - utcEpoch);

    if (s_tzMutex) {
        xSemaphoreGiveRecursive(s_tzMutex);
    }

    return offset;
}

int GlobalTime::getOffsetForTimezone(const char *timezoneLocation, int fallbackOffset) {
    if (!isTimeValid()) {
        return fallbackOffset;
    }

    const char *posixTz = getPosixTz(timezoneLocation);
    if (posixTz == nullptr) {
        DEBUG_PRINTF("GlobalTime: No POSIX TZ for '%s', keeping fallback offset %d\n", timezoneLocation, fallbackOffset);
        return fallbackOffset;
    }

    // Protect global TZ environment variable from concurrent access by background tasks
    if (s_tzMutex && xSemaphoreTakeRecursive(s_tzMutex, pdMS_TO_TICKS(500)) != pdTRUE) {
        Log.warningln("Failed to acquire TZ mutex in getOffsetForTimezone");
        return fallbackOffset;
    }

    const char *savedTz = getenv("TZ");
    String savedTzStr = savedTz ? String(savedTz) : "";

    setenv("TZ", posixTz, 1);
    tzset();
    const int offset = calculateActiveTimezoneOffset(m_unixEpoch);

    if (savedTzStr.length() > 0) {
        setenv("TZ", savedTzStr.c_str(), 1);
    } else {
        unsetenv("TZ");
    }
    tzset();

    if (s_tzMutex) {
        xSemaphoreGiveRecursive(s_tzMutex);
    }

    return offset;
}
