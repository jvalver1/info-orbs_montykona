#include "5ZoneWidget.h"
#include "5ZoneTranslations.h"
#include "DebugHelper.h"
#include "FlagImages.h"
#include "PosixTimezones.h"
#include "TFT_eSPI.h"
#include <ArduinoJson.h>
#include <ArduinoLog.h>

FiveZoneWidget::FiveZoneWidget(ScreenManager &manager, ConfigManager &config) : Widget(manager, config) {
    m_enabled = (INCLUDE_5ZONE == WIDGET_ON);
    m_time = nullptr;

    m_config.addConfigBool("FiveZoneWidget", "5zoEnabled", &m_enabled, t_enableWidget);
    m_config.addConfigBool("FiveZoneWidget", "showBizHours", &m_showBizHours, t_5zoneShowBizHours, false);

    // Array of default zone names, timezones, offsets, and flags from config.h
    const char *defaultNames[] = {ZONE_0_NAME, ZONE_1_NAME, ZONE_2_NAME, ZONE_3_NAME, ZONE_4_NAME};
    const char *defaultTZ[] = {ZONE_0_TIMEZONE, ZONE_1_TIMEZONE, ZONE_2_TIMEZONE, ZONE_3_TIMEZONE, ZONE_4_TIMEZONE};
    const int defaultOffsets[] = {ZONE_0_OFFSET, ZONE_1_OFFSET, ZONE_2_OFFSET, ZONE_3_OFFSET, ZONE_4_OFFSET};
    const char *defaultFlags[] = {ZONE_0_FLAG, ZONE_1_FLAG, ZONE_2_FLAG, ZONE_3_FLAG, ZONE_4_FLAG};

    for (int i = 0; i < MAX_ZONES; i++) {
        // Set default values from config.h (will be overridden if web config exists)
        m_timeZones[i].locName = defaultNames[i];
        m_timeZones[i].tzInfo = defaultTZ[i];
        m_timeZones[i].timeZoneOffset = defaultOffsets[i]; // Use hardcoded offset from config.h
        m_timeZones[i].flag = defaultFlags[i]; // Initialize flag

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
            m_timeZones[i].timeZoneOffset = defaultOffsets[i]; // Also reset offset
            m_timeZones[i].flag = defaultFlags[i]; // Also reset flag
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

// Draw a country flag using embedded JPG bitmap from Flagpedia CDN
// Flag will be drawn at (x, y) with dimensions (width x height)
void FiveZoneWidget::drawCountryFlag(const String &countryCode, int x, int y, int width, int height) {
    const uint8_t *flagData = getFlagStart(countryCode);
    uint32_t flagSize = getFlagSize(countryCode);

    if (flagData != nullptr && flagSize > 0) {
        m_manager.drawJpg(x, y, flagData, flagSize);
    } else {
        // Fallback: Draw country code text if flag bitmap not available
        m_manager.fillRect(x, y, width, height, TFT_DARKGREY);
        m_manager.drawString(countryCode.c_str(), x + width / 2, y + height / 2, 12, Align::MiddleCenter, TFT_WHITE, TFT_DARKGREY);
    }
}

void FiveZoneWidget::setup() {
    m_time = GlobalTime::getInstance();
}

void FiveZoneWidget::getTZoneOffset(int8_t zoneIndex) {
    TimeZone &zone = m_timeZones[zoneIndex];

    if (!m_time || !m_time->isTimeValid()) {
        DEBUG_PRINTF("Zone %d: System time not synced yet, deferring offset computation\n", zoneIndex);
        return;
    }

    zone.timeZoneOffset = m_time->getOffsetForTimezone(zone.tzInfo.c_str(), zone.timeZoneOffset);
    DEBUG_PRINTF("Zone %d (%s): active offset=%d sec (%d hours)\n",
                 zoneIndex, zone.tzInfo.c_str(),
                 zone.timeZoneOffset, zone.timeZoneOffset / 3600);

    // Refresh hourly to catch DST transitions
    zone.nextTimeZoneUpdate = m_time->getUnixEpoch() + 3600;
}

void FiveZoneWidget::update(bool force) {
    if (m_time == nullptr) {
        m_time = GlobalTime::getInstance();
    }
    m_time->updateTime(true);
    if (!m_time->isTimeValid()) {
        return;
    }
    int clockStamp = getClockStamp();

    if (clockStamp != m_clockStampU || force) {
        m_clockStampU = clockStamp;
        time_t lv_localEpoch = m_time->getUnixEpoch();

        // Compute timezone offsets using POSIX rules (no API needed)
        for (int i = 0; i < MAX_ZONES; i++) {
            TimeZone &zone = m_timeZones[i];
            if (zone.nextTimeZoneUpdate == 0 || (lv_localEpoch > zone.nextTimeZoneUpdate)) {
                getTZoneOffset(i);
            }
        }
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
    int currentSecond = m_time->getSecond();

    bool minuteChanged = (clockStamp != m_clockStampD);

    if (minuteChanged || force) {
        m_clockStampD = clockStamp;
        for (int i = 0; i < MAX_ZONES; i++) {
            displayZone(i, force);
        }
    }

    // Update seconds dot every second on all screens
    for (int i = 0; i < MAX_ZONES; i++) {
        if (currentSecond != m_lastSecond[i] || force) {
            m_manager.selectScreen(i);
            // Erase previous dot
            if (m_lastSecond[i] >= 0) {
                drawSecondsDot(i, m_lastSecond[i], m_backgroundColor);
            }
            // Draw new dot
            drawSecondsDot(i, currentSecond, TFT_CYAN);
            m_lastSecond[i] = currentSecond;
        }
    }
}

// Draw a filled rounded rectangle using fillRect + fillCircle for corners
void FiveZoneWidget::drawRoundedRect(int x, int y, int w, int h, int r, uint16_t color) {
    // Central body
    m_manager.fillRect(x + r, y, w - 2 * r, h, color);
    // Left and right strips
    m_manager.fillRect(x, y + r, r, h - 2 * r, color);
    m_manager.fillRect(x + w - r, y + r, r, h - 2 * r, color);
    // Four corner circles
    m_manager.fillCircle(x + r, y + r, r, color);
    m_manager.fillCircle(x + w - r - 1, y + r, r, color);
    m_manager.fillCircle(x + r, y + h - r - 1, r, color);
    m_manager.fillCircle(x + w - r - 1, y + h - r - 1, r, color);
}

// Draw/erase the seconds indicator dot on the border circle
void FiveZoneWidget::drawSecondsDot(int8_t displayIndex, int sec, uint16_t color) {
    // Map seconds (0-59) to angle in radians, starting from 12 o'clock (top)
    float angle = (sec / 60.0f) * 2.0f * PI - PI / 2.0f;
    int dotRadius = 4;
    int orbitRadius = 113; // Just inside the border circle
    int dotX = ScreenCenterX + (int) (orbitRadius * cos(angle));
    int dotY = ScreenCenterY + (int) (orbitRadius * sin(angle));
    m_manager.fillCircle(dotX, dotY, dotRadius, color);

    // When erasing, redraw the border circle to clean up any overlap
    if (color == m_backgroundColor) {
        uint16_t borderColor = TFT_CYAN;
        m_manager.drawCircle(ScreenCenterX, ScreenCenterY, 119, borderColor);
        m_manager.drawCircle(ScreenCenterX, ScreenCenterY, 118, borderColor);
        m_manager.drawCircle(ScreenCenterX, ScreenCenterY, 117, borderColor);
        m_manager.drawCircle(ScreenCenterX, ScreenCenterY, 116, borderColor);
    }
}

void FiveZoneWidget::displayZone(int8_t displayIndex, bool force) {
    // --- Layout positions (matching reference image) ---
    const int nameY = 51; // City name at top
    const int flagY = 89; // Flag below name (2px lower)
    const int clockY = 139; // Time center Y (2px lower)
    const int cardY = 114; // Top of the rounded rect card (2px lower)
    const int cardH = 50; // Card height
    const int cardW = 204; // Card width (wider for more side space)
    const int cardR = 4; // Card corner radius (more square)
    const int offsetY = 196; // GMT offset below the card (2px lower)
    const uint16_t cardColor = TFT_CYAN; // Clear blue
    const uint16_t timeColor = TFT_BLUE; // White text on blue

    String lv_displayHour = "";
    String lv_offsetStr = " ";
    int lv_ringColor;
    String lv_displayAM = "";
    time_t lv_unixEpoch;
    int lv_localDay;
    int lv_zoneDiff;
    int lv_hour;
    int lv_minute;
    int lv_day;
    int lv_weekday;
    int lv_hourD;

    m_manager.setFont(DEFAULT_FONT);
    m_manager.selectScreen(displayIndex);

    if (force) {
        m_manager.fillScreen(m_backgroundColor);
        // Draw 4-pixel border circle
        uint16_t borderColor = TFT_CYAN;
        m_manager.drawCircle(ScreenCenterX, ScreenCenterY, 119, borderColor);
        m_manager.drawCircle(ScreenCenterX, ScreenCenterY, 118, borderColor);
        m_manager.drawCircle(ScreenCenterX, ScreenCenterY, 117, borderColor);
        m_manager.drawCircle(ScreenCenterX, ScreenCenterY, 116, borderColor);
    }
    m_foregroundColor = m_workColour;
    m_manager.setFontColor(m_foregroundColor);

    TimeZone &zone = m_timeZones[displayIndex];

    if (m_time == nullptr || !m_time->isTimeValid()) {
        return;
    }

    DEBUG_PRINTF("Zone %d: name='%s', tzInfo='%s', offset=%d\n",
                 displayIndex,
                 zone.locName.c_str(),
                 zone.tzInfo.c_str(),
                 zone.timeZoneOffset);

    if (zone.locName != "") {
        m_localTimeZone.locName = "Local Time";
        m_localTimeZone.timeZoneOffset = m_time->getTimeZoneOffset();
        m_unixEpoch = m_time->getUnixEpoch();

        lv_unixEpoch = m_unixEpoch + zone.timeZoneOffset;
        lv_hour = hour(lv_unixEpoch);
        lv_minute = minute(lv_unixEpoch);
        lv_day = day(lv_unixEpoch);
        lv_weekday = weekday(lv_unixEpoch);

        lv_zoneDiff = zone.timeZoneOffset - m_localTimeZone.timeZoneOffset;
        // Round to nearest whole hour for display
        lv_hourD = (int) round((float) lv_zoneDiff / 3600.0f);

        if (lv_zoneDiff > 0) {
            lv_offsetStr = "+";
            lv_ringColor = m_afterLocalTzColour;
        } else if (lv_zoneDiff < 0) {
            lv_offsetStr = "-";
            lv_hourD = abs(lv_hourD);
            lv_ringColor = m_beforeLocalTzColour;
        } else {
            lv_offsetStr = "";
            lv_ringColor = m_sameLocalTzColour;
        }
        // Format as "GMT +N"
        String gmtPrefix = "GMT ";
        lv_offsetStr = gmtPrefix + lv_offsetStr + String(lv_hourD);

        // 12/24 hour format
        if (m_format == 0) {
            lv_displayHour = ((lv_hour < 10) ? "0" : "") + String(lv_hour);
        } else {
            lv_displayHour = String(hourFormat12(lv_unixEpoch));
            lv_displayAM = (isAM(lv_unixEpoch)) ? "AM" : "PM";
        }

        // --- City name ---
        m_manager.drawString(zone.locName.c_str(), ScreenCenterX, nameY, 18, Align::MiddleCenter);

        // --- Flag below city name ---
        int flagWidth = 40;
        int flagHeight = 30;
        if (force) {
            drawCountryFlag(zone.flag.c_str(), ScreenCenterX - flagWidth / 2, flagY - flagHeight / 2, flagWidth, flagHeight);
        }

        // --- Rounded rectangle card for time ---
        int cardX = ScreenCenterX - cardW / 2;
        if (force) {
            drawRoundedRect(cardX, cardY, cardW, cardH, cardR, cardColor);
        }

        // --- Time inside the card (use bold font) ---
        m_manager.setFont(ORBITRON_BOLD);
        String minuteStr = (lv_minute < 10) ? "0" + String(lv_minute) : String(lv_minute);
        String lv_displayTime = lv_displayHour + ":" + minuteStr;
        // Clear full card interior to prevent digit ghosting from non-monospaced font
        drawRoundedRect(cardX, cardY, cardW, cardH, cardR, cardColor);
        m_manager.drawString(lv_displayTime, ScreenCenterX, clockY, 42, Align::MiddleCenter, timeColor, cardColor);
        m_manager.setFont(DEFAULT_FONT);

        // --- AM/PM if 12h format ---
        if (lv_displayAM != "") {
            m_manager.drawString(lv_displayAM, ScreenCenterX + cardW / 2 - 22, clockY + 12, 12, Align::MiddleCenter, timeColor, cardColor);
        }

        // --- GMT offset below the card ---
        if (lv_zoneDiff != zone.m_zoneDiff || force) {
            m_manager.fillRect(ScreenCenterX - 60, offsetY - 12, 120, 24, m_backgroundColor);
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
    }
}

void FiveZoneWidget::buttonPressed(uint8_t buttonId, ButtonState state) {
    if (buttonId == BUTTON_OK && state == BTN_MEDIUM) {
        changeFormat();
    }
}

String FiveZoneWidget::getName() {
    return "5 Zone Clock";
}
