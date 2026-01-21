#include "5zonewidget/5ZoneWidget.h"
#include "DebugHelper.h"
#include "GlobalResources.h"
#include "MainHelper.h"
#include "clockwidget/ClockWidget.h"
#include "matrixwidget/MatrixWidget.h"
#include "mqttwidget/MQTTWidget.h"
#include "parqetwidget/ParqetWidget.h"
#include "stockwidget/StockWidget.h"
#include "weatherwidget/WeatherWidget.h"
#include "webdatawidget/WebDataWidget.h"
#include "eyeswidget/EyesWidget.h"
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
    DEBUG_PRINTF("🚀 Starting up...\n");
    DEBUG_PRINTF("PCB Version: %s\n", PCB_VERSION);

    wifiManager = new OrbsWiFiManager();
    config = new ConfigManager(*wifiManager);
    sm = new ScreenManager(tft);
    widgetSet = new WidgetSet(sm);

    // Pass references to MainHelper
    MainHelper::init(wifiManager, config, sm, widgetSet);
    MainHelper::watchdogReset();  // Reset after basic initialization
    
    MainHelper::setupLittleFS();
    MainHelper::watchdogReset();  // Reset after LittleFS mounting
    
    MainHelper::setupConfig();
    MainHelper::watchdogReset();  // Reset after config loading
    
    MainHelper::setupButtons();
    MainHelper::showWelcome();

    pinMode(BUSY_PIN, OUTPUT);
    DEBUG_PRINTF("Connecting to WiFi\n");
    MainHelper::watchdogReset();  // Reset before WiFi connection
    
    wifiWidget = new WifiWidget(*sm, *config, *wifiManager);
    wifiWidget->setup();
    MainHelper::watchdogReset();  // Reset after WiFi setup

    globalTime = GlobalTime::getInstance();
    addWidgets();
    MainHelper::watchdogReset();  // Reset after widget initialization
    
    config->setupWebPortal();
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
            widgetSet->initializeAllWidgetsData();
            MainHelper::setupWebPortalEndpoints();
        }
        globalTime->updateTime();

        MainHelper::checkButtons();

        widgetSet->updateCurrent();
        MainHelper::updateBrightnessByTime(globalTime->getHour24());
        widgetSet->drawCurrent();

        MainHelper::checkCycleWidgets();
        wifiManager->process();
        TaskManager::getInstance()->processAwaitingTasks();
        TaskManager::getInstance()->processTaskResponses();
    }
#ifdef MEMORY_DEBUG_INTERVAL
    ShowMemoryUsage::printSerial();
#endif
    MainHelper::restartIfNecessary();
}
