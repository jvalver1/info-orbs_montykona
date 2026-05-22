#include "StockWidget.h"
#include "CrashTrace.h"
#include "DebugHelper.h"
#include "StockTranslations.h"
#include "TaskFactory.h"
#include <ArduinoJson.h>
#include <iomanip>

#define WIDGET_PREFIX "[Stock]"

StockWidget::StockWidget(ScreenManager &manager, ConfigManager &config)
    : Widget(manager, config),
      m_drawTimer(addDrawRefreshFrequency(STOCK_DRAW_DELAY)),
      m_updateTimer(addUpdateRefreshFrequency(STOCK_UPDATE_DELAY)) {
    m_enabled = (INCLUDE_STOCK == WIDGET_ON);

    m_config.addConfigBool("StockWidget", "stocksEnabled", &m_enabled, t_enableWidget);
    m_config.addConfigString("StockWidget", "stockList", &m_stockList, 200, t_stockList);
    // Use heap allocation instead of VLA to prevent stack overflow with long stock lists
    char *stockList = new char[m_stockList.size() + 1];
    strcpy(stockList, m_stockList.c_str());

    m_config.addConfigComboBox("StockWidget", "stockchgFmt", &m_stockchangeformat, t_stockChangeFormats, t_stockChangeFormat, true);
    m_config.addConfigInt("StockWidget", "stockPaginate", &m_switchinterval, t_stockSwitchInterval, true);

    char *savePtr = nullptr;
    char *symbol = strtok_r(stockList, ",", &savePtr);
    m_stockCount = 0;
    while (symbol != nullptr) {
        if (m_stockCount >= MAX_STOCKS) {
            WIDGET_LOG_WARN(WIDGET_PREFIX, "MAX STOCKS UNABLE TO ADD MORE");
            break;
        }
        StockDataModel stockModel = StockDataModel();
        stockModel.setSymbol(String(symbol));
        m_stocks[m_stockCount] = stockModel;
        m_stockCount++;
        symbol = strtok_r(nullptr, ",", &savePtr);
    }
    delete[] stockList;
    m_pageCount = 1 + ((m_stockCount - 1) / NUM_SCREENS); // int division round up
    WIDGET_LOG_INFO(WIDGET_PREFIX, "StockWidget initialized");
    WIDGET_LOG_INFO(WIDGET_PREFIX, "StockWidget Pages: %d across %d symbols", m_pageCount, m_stockCount);
}

void StockWidget::setup() {
    if (m_stockCount == 0) {
        WIDGET_LOG_WARN(WIDGET_PREFIX, "No stock tickers available");
        return;
    }
    m_prevMillisSwitch = millis();
    WIDGET_HEAP_SNAP(WIDGET_PREFIX, "after_init");
}

void StockWidget::draw(bool force) {
    m_manager.setFont(DEFAULT_FONT);
    for (int8_t i = m_page * NUM_SCREENS; i < (m_page + 1) * NUM_SCREENS; i++) {
        int8_t displayIndex = i % NUM_SCREENS;
        if (!m_stocks[i].isInitialized() && !m_stocks[i].getSymbol().isEmpty() && m_stocks[i].getTicker().isEmpty()) {
            m_manager.selectScreen(displayIndex);
            m_manager.clearScreen(displayIndex);
            m_manager.setFontColor(TFT_WHITE, TFT_BLACK);
            m_manager.drawCentreString(I18n::get(t_loadingData), ScreenCenterX, ScreenCenterY, 16);
        } else if ((m_stocks[i].isChanged() || force) && !m_stocks[i].getSymbol().isEmpty()) {
            WIDGET_LOG_TRACE(WIDGET_PREFIX, "StockWidget::draw - %s", m_stocks[i].getSymbol().c_str());
            displayStock(displayIndex, m_stocks[i], TFT_WHITE, TFT_BLACK);
            m_stocks[i].setChangedStatus(false);
            m_stocks[i].setInitializationStatus(true);
        } else if (force) {
            m_manager.selectScreen(displayIndex);
            m_manager.clearScreen(displayIndex);
        }
    }

    if ((millis() - m_prevMillisSwitch >= (m_switchinterval * 1000)) && m_switchinterval > 0) {
        nextPage();
    }
}

void StockWidget::update(bool force) {
    WIDGET_HEAP_SNAP(WIDGET_PREFIX, "pre_update_fetches");
    // Queue requests for each stock
    for (int8_t i = 0; i < m_stockCount; i++) {
        WIDGET_LOG_TRACE(WIDGET_PREFIX, "StockWidget::update - %s", m_stocks[i].getSymbol().c_str());
        String url = "https://api.twelvedata.com/quote?apikey=e03fc53524454ab8b65d91b23c669cc5&symbol=" + m_stocks[i].getSymbol();

        StockDataModel *stock = &m_stocks[i];

        auto task = TaskFactory::createHttpGetTask(url, [this, stock](int httpCode, const String &response) {
            processResponse(*stock, httpCode, response);
        });

        if (TaskManager::getInstance()->addTask(std::move(task)) && i < m_stockCount - 1) {
            vTaskDelay(pdMS_TO_TICKS(STOCK_REQUEST_QUEUE_DELAY_MS));
        }
    }
    WIDGET_HEAP_SNAP(WIDGET_PREFIX, "post_update_fetches");
}

void StockWidget::processResponse(StockDataModel &stock, int httpCode, const String &response) {
    CrashTrace::mark("stock:process-response", stock.getSymbol());
    if (httpCode > 0) {
        JsonDocument filter;
        filter["close"] = true;
        filter["percent_change"] = true;
        filter["change"] = true;
        filter["fifty_two_week"]["high"] = true;
        filter["fifty_two_week"]["low"] = true;
        filter["name"] = true;
        filter["symbol"] = true;
        filter["currency"] = true;

        WIDGET_HEAP_SNAP(WIDGET_PREFIX, "pre-deserialization");
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, response, DeserializationOption::Filter(filter));
        WIDGET_HEAP_SNAP(WIDGET_PREFIX, "post-deserialization");

        if (!error) {
            float currentPrice = doc["close"].as<float>();
            if (currentPrice > 0.0) {
                stock.setCurrentPrice(currentPrice);
                stock.setPercentChange(doc["percent_change"].as<float>() / 100);
                stock.setPriceChange(doc["change"].as<float>());
                stock.setHighPrice(doc["fifty_two_week"]["high"].as<float>());
                stock.setLowPrice(doc["fifty_two_week"]["low"].as<float>());
                stock.setCompany(doc["name"].as<String>());
                stock.setTicker(doc["symbol"].as<String>());
                stock.setCurrencySymbol(doc["currency"].as<String>());
            } else {
                WIDGET_LOG_WARN(WIDGET_PREFIX, "skipping invalid data for: %s", stock.getSymbol().c_str());
            }
        } else {
            WIDGET_LOG_ERROR(WIDGET_PREFIX, "deserializeJson() failed: %s", error.c_str());
        }
    } else {
        WIDGET_LOG_ERROR(WIDGET_PREFIX, "HTTP request failed, error: %d", httpCode);
    }
}

void StockWidget::changeMode() {
    update(true);
}

void StockWidget::buttonPressed(uint8_t buttonId, ButtonState state) {
    if (buttonId == BUTTON_OK && state == BTN_SHORT)
        nextPage();
    else if (buttonId == BUTTON_OK && state == BTN_LONG)
        changeMode();
}

void StockWidget::displayStock(int8_t displayIndex, StockDataModel &stock, uint32_t backgroundColor, uint32_t textColor) {
    WIDGET_LOG_TRACE(WIDGET_PREFIX, "displayStock - %s ~ %s", stock.getSymbol().c_str(), stock.getCurrentPrice(2).c_str());
    if (stock.getCurrentPrice() == 0.0) {
        // There isn't any data to display yet
        return;
    }
    m_manager.selectScreen(displayIndex);
    m_manager.clearScreen(displayIndex);

    // Calculate center positions
    int screenWidth = SCREEN_SIZE;
    int centre = 120;
    int arrowOffsetX = 0;
    int arrowOffsetY = -109;

    // Outputs
    m_manager.fillRect(0, 70, screenWidth, 49, TFT_WHITE);
    m_manager.fillRect(0, 111, screenWidth, 20, TFT_LIGHTGREY);
    int smallFontSize = 11;
    int bigFontSize = 29;
    m_manager.setFontColor(TFT_WHITE, TFT_BLACK);
    m_manager.drawCentreString(i18n(t_stock52week), centre, 185, smallFontSize);
    m_manager.drawCentreString(i18nStr(t_highShort) + ": " + stock.getCurrencySymbol() + stock.getHighPrice(), centre, 200, smallFontSize);
    m_manager.drawCentreString(i18nStr(t_lowShort) + ": " + stock.getCurrencySymbol() + stock.getLowPrice(), centre, 215, smallFontSize);
    m_manager.setFontColor(TFT_BLACK, TFT_LIGHTGREY);
    m_manager.drawString(stock.getCompany(), centre, 121, smallFontSize, Align::MiddleCenter);
    if (stock.getPercentChange() < 0.0) {
        m_manager.setFontColor(TFT_RED, TFT_BLACK);
        m_manager.fillTriangle(110 + arrowOffsetX, 120 + arrowOffsetY, 130 + arrowOffsetX, 120 + arrowOffsetY, 120 + arrowOffsetX, 132 + arrowOffsetY, TFT_RED);
        m_manager.drawArc(centre, centre, 120, 118, 0, 360, TFT_RED, TFT_RED);
    } else {
        m_manager.setFontColor(TFT_GREEN, TFT_BLACK);
        m_manager.fillTriangle(110 + arrowOffsetX, 132 + arrowOffsetY, 130 + arrowOffsetX, 132 + arrowOffsetY, 120 + arrowOffsetX, 120 + arrowOffsetY, TFT_GREEN);
        m_manager.drawArc(centre, centre, 120, 118, 0, 360, TFT_GREEN, TFT_GREEN);
    }
    if (!m_stockchangeformat) {
        m_manager.drawString(stock.getPercentChange(2) + "%", centre, 48, bigFontSize, Align::MiddleCenter);
    } else {
        m_manager.drawString(stock.getCurrencySymbol() + stock.getPriceChange(2), centre, 48, bigFontSize, Align::MiddleCenter);
    }
    // Draw stock data
    m_manager.setFontColor(TFT_BLACK, TFT_WHITE);

    m_manager.drawString(stock.getTicker(), centre, 92, bigFontSize, Align::MiddleCenter);
    m_manager.setFontColor(TFT_WHITE, TFT_BLACK);

    m_manager.drawString(stock.getCurrencySymbol() + stock.getCurrentPrice(2), centre, 155, bigFontSize, Align::MiddleCenter);
}

void StockWidget::nextPage() {
    if (m_pageCount <= 1)
        return;
    // Reset the timer for the next page if we just switched manually
    m_prevMillisSwitch = millis();
    m_page = (m_page + 1) % m_pageCount;
    WIDGET_LOG_TRACE(WIDGET_PREFIX, "StockWidget Page: %d", m_page + 1);
    draw(true);
}

String StockWidget::getName() {
    return "Stock";
}
