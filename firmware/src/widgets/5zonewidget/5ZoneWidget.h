#ifndef FIVE_ZONE_WIDGET_H
#define FIVE_ZONE_WIDGET_H

#include "GlobalTime.h"
#include "Widget.h"
#include "config_helper.h"

#define MAX_ZONES 5

// Working hours configuration (default if not specified in config.h)
#define DEFAULT_WORK_HOUR_START 9 // 9 AM
#define DEFAULT_WORK_HOUR_END 17 // 5 PM

// Colors for different states
#define BG_COLOR TFT_BLACK
#define WORK_FG_COLOR TFT_WHITE
#define AFTER_FG_COLOR TFT_RED
#define WEEKEND_FG_COLOR TFT_YELLOW
#define BEFORE_LOCAL_TZ TFT_RED
#define SAME_LOCAL_TZ TFT_BLACK
#define AFTER_LOCAL_TZ TFT_GREEN

struct TimeZone {
    std::string locName = "";
    std::string tzInfo = "";
    std::string flag = "";  // Country flag emoji or code
    int timeZoneOffset = -1;
    time_t nextTimeZoneUpdate = 0;
    int m_workStart = 9; // Work start hour for this zone
    int m_workEnd = 17; // Work end hour for this zone
    String m_lastDateIndicator = "";
    String m_lastDisplayAM = "";
    int m_zoneDiff = 99;
};

class FiveZoneWidget : public Widget {
public:
    FiveZoneWidget(ScreenManager &manager, ConfigManager &config);
    void setup() override;
    void update(bool force) override;
    void draw(bool force) override;
    void buttonPressed(uint8_t buttonId, ButtonState state) override;
    String getName() override;

private:
    int getClockStamp();
    void getTZoneOffset(int8_t zoneIndex);
    void displayZone(int8_t displayIndex, bool force);
    bool isWeekend(int weekday) { return weekday == 1 || weekday == 7; }
    void changeFormat();
    void drawCountryFlag(const String& countryCode, int x, int y, int width, int height);

    TimeZone m_timeZones[MAX_ZONES];
    TimeZone m_localTimeZone;
    GlobalTime *m_time;
    time_t m_unixEpoch = 0;
    int m_clockStampU = -1;
    int m_clockStampD = -1;
    uint16_t m_backgroundColor = BG_COLOR;
    uint16_t m_foregroundColor;
    uint16_t m_workColour = WORK_FG_COLOR;
    uint16_t m_afterWorkColour = AFTER_FG_COLOR;
    uint16_t m_weekendColor = WEEKEND_FG_COLOR;
    uint16_t m_beforeLocalTzColour = BEFORE_LOCAL_TZ;
    uint16_t m_sameLocalTzColour = SAME_LOCAL_TZ;
    uint16_t m_afterLocalTzColour = AFTER_LOCAL_TZ;

    std::string m_timezoneLocation = TIMEZONE_API_LOCATION;
    int m_format = CLOCK_FORMAT;
    bool m_showBizHours;
};

#endif // FIVE_ZONE_WIDGET_H
