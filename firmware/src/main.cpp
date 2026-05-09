#include "5zonewidget/5ZoneWidget.h"
#include "CrashTrace.h"
#include "DebugHelper.h"
#include "GlobalResources.h"
#include "MainHelper.h"
#include "clockwidget/ClockWidget.h"
#include "eyeswidget/EyesWidget.h"
#include "matrixwidget/MatrixWidget.h"
#include "mqttwidget/MQTTWidget.h"
#include "parqetwidget/ParqetWidget.h"
#include "stockwidget/StockWidget.h"
#include "weatherwidget/WeatherWidget.h"
#include "webdatawidget/WebDataWidget.h"
#include "wifiwidget/WifiWidget.h"
#include <ArduinoLog.h>

TFT_eSPI tft = TFT_eSPI();

GlobalTime *globalTime{nullptr};
WifiWidget *wifiWidget{nullptr};
ScreenManager *sm{nullptr};
ConfigManager *config{nullptr};
OrbsWiFiManager *wifiManager{nullptr};
WidgetSet *widgetSet{nullptr};

void addWidgets() {
    // Always add clock
    widgetSet->add(new ClockWidget(*sm, *config));

#if INCLUDE_WEATHER != WIDGET_DISABLED
    widgetSet->add(new WeatherWidget(*sm, *config));
#endif

#if INCLUDE_STOCK != WIDGET_DISABLED
    widgetSet->add(new StockWidget(*sm, *config));
#endif
#if INCLUDE_PARQET != WIDGET_DISABLED
    widgetSet->add(new ParqetWidget(*sm, *config));
#endif
#if INCLUDE_WEBDATA != WIDGET_DISABLED
    #ifdef WEB_DATA_WIDGET_URL
    widgetSet->add(new WebDataWidget(*sm, *config, WEB_DATA_WIDGET_URL));
    #endif
    #ifdef WEB_DATA_STOCK_WIDGET_URL
    widgetSet->add(new WebDataWidget(*sm, *config, WEB_DATA_STOCK_WIDGET_URL));
    #endif
#endif
#if INCLUDE_MQTT != WIDGET_DISABLED
    widgetSet->add(new MQTTWidget(*sm, *config));
#endif
#if INCLUDE_5ZONE != WIDGET_DISABLED
    widgetSet->add(new FiveZoneWidget(*sm, *config));
#endif
#if INCLUDE_MATRIXSCREEN != WIDGET_DISABLED
    widgetSet->add(new MatrixWidget(*sm, *config));
#endif
#if INCLUDE_EYES != WIDGET_DISABLED
    widgetSet->add(new EyesWidget(*sm, *config));
#endif
}

void setup() {
    // Initialize global resources
    initializeGlobalResources();

#ifdef SERIAL_INTERFACE_INIT_DELAY
    // Add a delay to allow the serial interface to initialize
    delay(SERIAL_INTERFACE_INIT_DELAY);
#endif

    Serial.begin(115200);

    // Clear the serial buffer of any garbage
    while (Serial.available() > 0) {
        Serial.read();
    }

#ifdef LOG_TIMESTAMP
    Log.setPrefix(MainHelper::printPrefix);
#endif
    Log.begin(LOG_LEVEL, &Serial);
    CrashTrace::printLastReset();
    CrashTrace::mark("setup:start");
    DEBUG_PRINTF("🚀 Starting up...\n");
    DEBUG_PRINTF("PCB Version: %s\n", PCB_VERSION);

    wifiManager = new OrbsWiFiManager();
    CrashTrace::mark("setup:wifi-manager");
    config = new ConfigManager(*wifiManager);
    CrashTrace::mark("setup:config-manager");
    sm = new ScreenManager(tft);
    CrashTrace::mark("setup:screen-manager");
    widgetSet = new WidgetSet(sm);

    // Pass references to MainHelper
    MainHelper::init(wifiManager, config, sm, widgetSet);
    MainHelper::watchdogReset(); // Reset after basic initialization

    MainHelper::setupLittleFS();
    CrashTrace::mark("setup:littlefs");
    MainHelper::watchdogReset(); // Reset after LittleFS mounting

    MainHelper::setupConfig();
    CrashTrace::mark("setup:config");
    MainHelper::watchdogReset(); // Reset after config loading

    MainHelper::setupButtons();
    MainHelper::showWelcome();
    CrashTrace::mark("setup:welcome");

    pinMode(BUSY_PIN, OUTPUT);
    DEBUG_PRINTF("Connecting to WiFi\n");
    MainHelper::watchdogReset(); // Reset before WiFi connection

    wifiWidget = new WifiWidget(*sm, *config, *wifiManager);
    wifiWidget->setup();
    CrashTrace::mark("setup:wifi-widget");
    MainHelper::watchdogReset(); // Reset after WiFi setup

    addWidgets();
    CrashTrace::mark("setup:widgets-added");
    globalTime = GlobalTime::getInstance();
    CrashTrace::mark("setup:global-time");
    globalTime->updateTime(true);
    MainHelper::watchdogReset(); // Reset after widget initialization

    config->setupWebPortal();
    CrashTrace::mark("setup:complete");
    MainHelper::resetCycleTimer();
}

void loop() {
    MainHelper::watchdogReset();
    if (wifiWidget->isConnected() == false) {
        wifiWidget->update();
        wifiWidget->draw();
        widgetSet->setClearScreensOnDrawCurrent(); // Clear screen after wifiWidget
        delay(100);
    } else {
        if (!widgetSet->initialUpdateDone()) {
            CrashTrace::mark("loop:initial-update");
            widgetSet->initializeAllWidgetsData();
            MainHelper::setupWebPortalEndpoints();
        }
        CrashTrace::mark("loop:time");
        globalTime->updateTime();

        CrashTrace::mark("loop:buttons");
        MainHelper::checkButtons();

        CrashTrace::mark("loop:update-current", widgetSet->getCurrent()->getName());
        widgetSet->updateCurrent();
        MainHelper::updateBrightnessByTime(globalTime->getHour24());
        CrashTrace::mark("loop:draw-current", widgetSet->getCurrent()->getName());
        widgetSet->drawCurrent();

        CrashTrace::mark("loop:cycle");
        MainHelper::checkCycleWidgets();
        wifiManager->process();
        CrashTrace::mark("loop:tasks-awaiting");
        TaskManager::getInstance()->processAwaitingTasks();
        CrashTrace::mark("loop:task-responses");
        TaskManager::getInstance()->processTaskResponses();
    }
#ifdef MEMORY_DEBUG_INTERVAL
    ShowMemoryUsage::printSerial();
#endif
    MainHelper::restartIfNecessary();
}
