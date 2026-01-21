#include "5ZoneWidget.h"
#include "DebugHelper.h"
#include "5ZoneTranslations.h"
#include <ArduinoJson.h>
#include <ArduinoLog.h>

FiveZoneWidget::FiveZoneWidget(ScreenManager &manager, ConfigManager &config) : Widget(manager, config) {
    m_enabled = (INCLUDE_5ZONE == WIDGET_ON);
    m_time = GlobalTime::getInstance();

    m_config.addConfigBool("FiveZoneWidget", "5zoEnabled", & m_enabled, t_enableWidget);
    m_config.addConfigBool("FiveZoneWidget", "showBizHours", &m_showBizHours, t_5zoneShowBizHours, false);

    // Array of default zone names, timezones, offsets, and flags from config.h
    const char* defaultNames[] = {ZONE_0_NAME, ZONE_1_NAME, ZONE_2_NAME, ZONE_3_NAME, ZONE_4_NAME};
    const char* defaultTZ[] = {ZONE_0_TIMEZONE, ZONE_1_TIMEZONE, ZONE_2_TIMEZONE, ZONE_3_TIMEZONE, ZONE_4_TIMEZONE};
    const int defaultOffsets[] = {ZONE_0_OFFSET, ZONE_1_OFFSET, ZONE_2_OFFSET, ZONE_3_OFFSET, ZONE_4_OFFSET};
    const char* defaultFlags[] = {ZONE_0_FLAG, ZONE_1_FLAG, ZONE_2_FLAG, ZONE_3_FLAG, ZONE_4_FLAG};


    for (int i = 0; i < MAX_ZONES; i++) {
        // Set default values from config.h (will be overridden if web config exists)
        m_timeZones[i].locName = defaultNames[i];
        m_timeZones[i].tzInfo = defaultTZ[i];
        m_timeZones[i].timeZoneOffset = defaultOffsets[i];  // Use hardcoded offset from config.h
        m_timeZones[i].flag = defaultFlags[i];  // Initialize flag
        
        const char *zoneName = strdup((String("5zoZoneName") + String(i)).c_str());
        const char *zoneDesc = strdup((i18nStr(t_5zoneDesc) + " " + String(i) + ": ").c_str());
        m_config.addConfigString("FiveZoneWidget", zoneName, &m_timeZones[i].locName, 50, zoneDesc, false);

        const char *zoneTZ = strdup((String("5zoZoneInfo") + String(i)).c_str());
        const char *zoneTZDesc = strdup((i18nStr(t_5zoneTZDesc) + " " + String(i) + ": ").c_str());
        m_config.addConfigString("FiveZoneWidget", zoneTZ, &m_timeZones[i].tzInfo, 50, zoneTZDesc, false);
        
        // Validate loaded timezone identifier - if it's a short abbreviation (4 chars or less),
        // it's likely old config using GMT, CET, etc. Replace with IANA identifier from config.h
        if (m_timeZones[i].tzInfo.length() <= 4 && strlen(defaultTZ[i]) > 4) {
            DEBUG_PRINTF("Zone %d has invalid timezone '%s', replacing with '%s'\n", 
                          i, m_timeZones[i].tzInfo.c_str(), defaultTZ[i]);
            m_timeZones[i].locName = defaultNames[i];
            m_timeZones[i].tzInfo = defaultTZ[i];
            m_timeZones[i].timeZoneOffset = defaultOffsets[i];  // Also reset offset
            m_timeZones[i].flag = defaultFlags[i];  // Also reset flag
        }
        
        DEBUG_PRINTF("Zone %d initialized: name='%s', flag='%s', offset=%d seconds (%d hours)\n", 
                     i, m_timeZones[i].locName.c_str(), m_timeZones[i].flag.c_str(),
                     m_timeZones[i].timeZoneOffset, m_timeZones[i].timeZoneOffset / 3600);
    }

    for (int i = 0; i < MAX_ZONES; i++) {
        const char *zoneWorkStart = strdup((String("5zoZoneWstart") + String(i)).c_str());
        const char *zoneWorkStartDesc = strdup((i18nStr(t_5zoneWorkStartDesc) + " " + String(i) + ": ").c_str());
        m_config.addConfigInt("FiveZoneWidget", zoneWorkStart, &m_timeZones[i].m_workStart, zoneWorkStartDesc, true);

        const char *zoneWorkEnd = strdup((String("5zoZoneWend") + String(i)).c_str());
        const char *zoneWorkEndDesc = strdup((i18nStr(t_5zoneWorkEndDesc) + " " + String(i) + ": ").c_str());
        m_config.addConfigInt("FiveZoneWidget", zoneWorkEnd, &m_timeZones[i].m_workEnd, zoneWorkEndDesc, true);
    }
    m_format = m_config.getConfigInt("clockFormat", 0);
}

// Draw a simplified country flag using graphic primitives
// Flag will be drawn at (x, y) with dimensions (width x height)
void FiveZoneWidget::drawCountryFlag(const String& countryCode, int x, int y, int width, int height) {
    String code = countryCode;
    code.toUpperCase();
    
    if (code == "GB") {
        // UK: Union Jack with diagonal crosses
        m_manager.fillRect(x, y, width, height, TFT_BLUE);
        
        // White diagonal stripes (St. Andrew's cross)
        // Top-left to bottom-right
        for (int i = 0; i < width; i++) {
            int yPos = y + (i * height) / width;
            m_manager.fillRect(x + i, yPos - 2, 1, 5, TFT_WHITE);
        }
        // Top-right to bottom-left
        for (int i = 0; i < width; i++) {
            int yPos = y + height - (i * height) / width;
            m_manager.fillRect(x + i, yPos - 2, 1, 5, TFT_WHITE);
        }
        
        // Red diagonal stripes (St. Patrick's cross)
        // Top-left to bottom-right
        for (int i = 0; i < width; i++) {
            int yPos = y + (i * height) / width;
            m_manager.fillRect(x + i, yPos, 1, 1, TFT_RED);
        }
        // Top-right to bottom-left
        for (int i = 0; i < width; i++) {
            int yPos = y + height - (i * height) / width;
            m_manager.fillRect(x + i, yPos, 1, 1, TFT_RED);
        }
        
        // White cross border (St. George's cross border)
        m_manager.fillRect(x + width/2 - 3, y, 6, height, TFT_WHITE);
        m_manager.fillRect(x, y + height/2 - 3, width, 6, TFT_WHITE);
        
        // Red cross (St. George's cross)
        m_manager.fillRect(x + width/2 - 2, y, 4, height, TFT_RED);
        m_manager.fillRect(x, y + height/2 - 2, width, 4, TFT_RED);
        
    } else if (code == "ES") {
        // Spain: Horizontal stripes (Red-Yellow-Red)
        int stripeHeight = height / 4;
        m_manager.fillRect(x, y, width, stripeHeight, TFT_RED);
        m_manager.fillRect(x, y + stripeHeight, width, stripeHeight * 2, TFT_YELLOW);
        m_manager.fillRect(x, y + stripeHeight * 3, width, stripeHeight, TFT_RED);
        
    } else if (code == "BR") {
        // Brazil: Green background with yellow diamond and blue globe
        m_manager.fillRect(x, y, width, height, TFT_GREEN);
        
        // Yellow diamond (rhombus) using two triangles
        int margin = 3;
        // Top triangle
        m_manager.fillTriangle(x + width/2, y + margin, 
                               x + margin, y + height/2, 
                               x + width - margin, y + height/2, 
                               TFT_YELLOW);
        // Bottom triangle
        m_manager.fillTriangle(x + margin, y + height/2, 
                               x + width - margin, y + height/2, 
                               x + width/2, y + height - margin, 
                               TFT_YELLOW);
        
        // Blue globe
        m_manager.fillCircle(x + width/2, y + height/2, height/5, TFT_BLUE);
        
    } else if (code == "US") {
        // USA: Simplified with blue canton and red/white stripes
        int stripeHeight = height / 4;
        // Red and white stripes
        m_manager.fillRect(x, y, width, stripeHeight, TFT_RED);
        m_manager.fillRect(x, y + stripeHeight, width, stripeHeight, TFT_WHITE);
        m_manager.fillRect(x, y + stripeHeight * 2, width, stripeHeight, TFT_RED);
        m_manager.fillRect(x, y + stripeHeight * 3, width, stripeHeight, TFT_WHITE);
        // Blue canton (top-left)
        m_manager.fillRect(x, y, width/2, height/2, TFT_BLUE);
        
    } else if (code == "JP") {
        // Japan: White background with red circle
        m_manager.fillRect(x, y, width, height, TFT_WHITE);
        int radius = height / 3;
        m_manager.fillCircle(x + width/2, y + height/2, radius, TFT_RED);
        
    } else {
        // Default: Draw country code text if flag not implemented
        m_manager.fillRect(x, y, width, height, TFT_DARKGREY);
        m_manager.drawString(countryCode.c_str(), x + width/2, y + height/2, 12, Align::MiddleCenter, TFT_WHITE, TFT_DARKGREY);
    }
}

void FiveZoneWidget::setup() {
}

void FiveZoneWidget::getTZoneOffset(int8_t zoneIndex) {

    TimeZone &zone = m_timeZones[zoneIndex];
    HTTPClient http;
    http.begin(String(TIMEZONE_API_URL) + "?timeZone=" + String(zone.tzInfo.c_str()));

    DEBUG_PRINTF("Fetching timezone for zone %d: %s\n", zoneIndex, zone.tzInfo.c_str());

    int httpCode = http.GET();
    if (httpCode > 0) {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, http.getString());
        if (!error) {
            zone.timeZoneOffset = doc["currentUtcOffset"]["seconds"].as<int>();
            DEBUG_PRINTF("Zone %d offset fetched: %d seconds (%d hours)\n", 
                         zoneIndex, zone.timeZoneOffset, zone.timeZoneOffset / 3600);
            
            if (doc["hasDayLightSaving"].as<bool>()) {
                String dstStart = doc["dstInterval"]["dstStart"].as<String>();
                String dstEnd = doc["dstInterval"]["dstEnd"].as<String>();
                bool dstActive = doc["isDayLightSavingActive"].as<bool>();
                tmElements_t m_temp_t;
                if (dstActive) {
                    m_temp_t.Year = dstEnd.substring(0, 4).toInt() - 1970;
                    m_temp_t.Month = dstEnd.substring(5, 7).toInt();
                    m_temp_t.Day = dstEnd.substring(8, 10).toInt();
                    m_temp_t.Hour = dstEnd.substring(11, 13).toInt();
                    m_temp_t.Minute = dstEnd.substring(14, 16).toInt();
                    m_temp_t.Second = dstEnd.substring(17, 19).toInt();
                } else {
                    m_temp_t.Year = dstStart.substring(0, 4).toInt() - 1970;
                    m_temp_t.Month = dstStart.substring(5, 7).toInt();
                    m_temp_t.Day = dstStart.substring(8, 10).toInt();
                    m_temp_t.Hour = dstStart.substring(11, 13).toInt();
                    m_temp_t.Minute = dstStart.substring(14, 16).toInt();
                    m_temp_t.Second = dstStart.substring(17, 19).toInt();
                }
                zone.nextTimeZoneUpdate = makeTime(m_temp_t) + random(5 * 60); // Randomize update by 5 minutes to avoid flooding the API;
            }
        } else {
            Log.errorln("Zone %d: Deserialization error on timezone offset API response", zoneIndex);
        }
    } else {
        Log.errorln("Zone %d: Failed to get timezone offset from API (HTTP code: %d)", zoneIndex, httpCode);
    }
}

void FiveZoneWidget::update(bool force) {
    m_time->updateTime(true);
    int clockStamp = getClockStamp();

    if (clockStamp != m_clockStampU || force) {
        m_clockStampU = clockStamp;
        time_t lv_localEpoch = m_time->getUnixEpoch();

        // API calls disabled - using hardcoded offsets from config.h
        // for (int i = 0; i < MAX_ZONES; i++) {
        //     TimeZone &zone = m_timeZones[i];
        //     if (zone.timeZoneOffset == -1 || (zone.nextTimeZoneUpdate > 0 && lv_localEpoch > zone.nextTimeZoneUpdate)) {
        //         getTZoneOffset(i);
        //         delay(100);
        //     }
        // }
    }
}

void FiveZoneWidget::changeFormat() {
    GlobalTime *time = GlobalTime::getInstance();
    m_format++;
    if (m_format > 1)
        m_format = 0;
    m_manager.clearAllScreens();
    update(true);
    draw(true);
}

int FiveZoneWidget::getClockStamp() {
    return m_time->getHour24() * 100 + m_time->getMinute();
}

void FiveZoneWidget::draw(bool force) {
    int clockStamp = getClockStamp();

    if (clockStamp != m_clockStampD || force) {
        m_clockStampD = clockStamp;
        for (int i = 0; i < MAX_ZONES; i++) {
            displayZone(i, force);
        }
    }
}

void FiveZoneWidget::displayZone(int8_t displayIndex, bool force) {
    const int nameY = 50; // Zone name at top
    const int dateY = 75; // Date indicator below name
    const int clockY = 115; // Time in middle
    const int ampmY = 175; // AM/PM indicator
    const int offsetY = 200; // Offset at bottom
    String lv_displayHour = "";
    String lv_offsetStr = " ";
    int lv_ringColor;
    String lv_dateIndicator = "";
    String lv_displayAM = "";
    time_t lv_unixEpoch;
    int lv_localDay;
    int lv_zoneDiff;
    int lv_hour;
    int lv_minute;
    int lv_day;
    int lv_weekday;
    int lv_hourD;
    int lv_minuteD;

    m_manager.setFont(DEFAULT_FONT);
    m_manager.selectScreen(displayIndex);

    if (force)
        m_manager.fillScreen(m_backgroundColor);
    m_foregroundColor = m_workColour;
    m_manager.setFontColor(m_foregroundColor);

    TimeZone &zone = m_timeZones[displayIndex];
    
    // Debug logging to diagnose timezone issues
    DEBUG_PRINTF("Zone %d: name='%s', tzInfo='%s', offset=%d\n", 
                 displayIndex, 
                 zone.locName.c_str(), 
                 zone.tzInfo.c_str(), 
                 zone.timeZoneOffset);
    
    if (zone.locName != "") {
        // Get Orb (local) time information
        m_localTimeZone.locName = "Local Time";
        m_localTimeZone.timeZoneOffset = m_time->getTimeZoneOffset();
        m_unixEpoch = m_time->getUnixEpoch();

        // Get Time information for this TZ
        lv_unixEpoch = m_unixEpoch + zone.timeZoneOffset - m_localTimeZone.timeZoneOffset;
        lv_hour = hour(lv_unixEpoch);
        lv_minute = minute(lv_unixEpoch);
        lv_day = day(lv_unixEpoch);
        lv_weekday = weekday(lv_unixEpoch);

        // Calculate offset from local time
        lv_zoneDiff = zone.timeZoneOffset - m_localTimeZone.timeZoneOffset; // Difference between target UTC offset and local UTC offset
        lv_hourD = lv_zoneDiff / 3600;
        lv_minuteD = (abs(lv_zoneDiff) / 60) % 60;  // FIX: Use abs() to avoid negative minutes

        // calculate if day offset
        if (lv_zoneDiff > 0) {
            lv_offsetStr = "+";
            lv_ringColor = m_afterLocalTzColour;
        } else if (lv_zoneDiff < 0) {
            lv_offsetStr = "-";
            lv_hourD = abs(lv_hourD);  // Use abs for consistent formatting
            lv_ringColor = m_beforeLocalTzColour;
        } else {
            lv_offsetStr = "";
            lv_ringColor = m_sameLocalTzColour;
        }
        lv_offsetStr = lv_offsetStr + ((lv_hourD < 10) ? "0" : "") + String(lv_hourD) + ":" + ((lv_minuteD < 10) ? "0" : "") + String(lv_minuteD);

        // calculate if crossing date line
        lv_localDay = m_time->getDay();
        if (lv_localDay != lv_day) {
            if (lv_unixEpoch > m_unixEpoch)
                lv_dateIndicator = "+1d";
            else
                lv_dateIndicator = "-1d";
        }

        // 12/24 hour formate and AM/PM indicator
        if (m_format == 0) {
            lv_displayHour = ((lv_hour < 10) ? "0" : "") + String(lv_hour);
            lv_displayAM = "";
        } else {
            lv_displayHour = String(hourFormat12(lv_unixEpoch));
            lv_displayAM = (isAM(lv_unixEpoch)) ? "AM" : "PM";
        }

        m_manager.drawString(zone.locName.c_str(), ScreenCenterX, nameY, 18, Align::MiddleCenter);

        if (lv_dateIndicator != zone.m_lastDateIndicator || force) {
            m_manager.fillRect(ScreenCenterX - 80, ampmY - 10, 45, 22, m_backgroundColor);
            m_manager.drawString(lv_dateIndicator, ScreenCenterX - 60, ampmY, 16, Align::MiddleCenter);
            zone.m_lastDateIndicator = lv_dateIndicator;
        }

        if (lv_displayAM != zone.m_lastDisplayAM || force) {
            m_manager.fillRect(ScreenCenterX + 43, ampmY - 10, 37, 22, m_backgroundColor);
            m_manager.drawString(lv_displayAM, ScreenCenterX + 60, ampmY, 16, Align::MiddleCenter);
            zone.m_lastDisplayAM = lv_displayAM;
        }

        // Draw flag between time and offset
        int flagY = offsetY - 25;  // Position flag above offset
        int flagWidth = 40;
        int flagHeight = 20;
        if (force) {
            m_manager.fillRect(ScreenCenterX - flagWidth/2 - 2, flagY - flagHeight/2 - 2, flagWidth + 4, flagHeight + 4, m_backgroundColor);
            drawCountryFlag(zone.flag.c_str(), ScreenCenterX - flagWidth/2, flagY - flagHeight/2, flagWidth, flagHeight);
        }

        if (lv_zoneDiff != zone.m_zoneDiff || force) {
            m_manager.fillRect(ScreenCenterX - 35, offsetY - 10, 72, 22, m_backgroundColor);
            if (zone.timeZoneOffset == -1)
                m_manager.setFontColor(TFT_RED);
            m_manager.drawString(lv_offsetStr, ScreenCenterX, offsetY, 16, Align::MiddleCenter);
            m_manager.setFontColor(m_foregroundColor);
            zone.m_zoneDiff = lv_zoneDiff;
        }

        if (m_showBizHours) {
            m_manager.drawArc(120, 120, 120, 115, 0, 360, lv_ringColor, m_backgroundColor);
            if (isWeekend(lv_weekday)) {
                m_foregroundColor = m_weekendColor;
                m_manager.setFontColor(m_foregroundColor);
            } else {
                if (m_showBizHours) {
                    if (lv_hour < zone.m_workStart || lv_hour >= zone.m_workEnd) {
                        m_foregroundColor = m_afterWorkColour;
                        m_manager.setFontColor(m_foregroundColor);
                    }
                }
            }
        }

        String minuteStr = (lv_minute < 10) ? "0" + String(lv_minute) : String(lv_minute);
        String lv_displayTime = lv_displayHour + ":" + minuteStr;
        m_manager.fillRect(14, 82, 215, 69, m_backgroundColor);
        m_manager.drawString(lv_displayTime, ScreenCenterX, clockY, 62, Align::MiddleCenter);
    }
}

void FiveZoneWidget ::buttonPressed(uint8_t buttonId, ButtonState state) {
    if (buttonId == BUTTON_OK && state == BTN_MEDIUM) {
        changeFormat();
    }
}

String FiveZoneWidget ::getName() {
    return "5 Zone Clock";
}
