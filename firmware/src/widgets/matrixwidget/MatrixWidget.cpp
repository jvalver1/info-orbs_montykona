#include "MatrixWidget.h"
#include "CrashTrace.h"
#include "MatrixTranslations.h"

MatrixWidget::MatrixWidget(ScreenManager &manager, ConfigManager &config) : Widget(manager, config) {
    m_enabled = (INCLUDE_MATRIXSCREEN == WIDGET_ON);
    m_config.addConfigBool("MatrixWidget", "mtxEnabled", &m_enabled, t_enableWidget);
    m_config.addConfigBool("MatrixWidget", "mtxBigFont", &m_bigFont, t_matrixBigFont, false);
    m_config.addConfigColor("MatrixWidget", "mtxTextColor", &m_textColor, t_matrixTextColor, false);
    m_config.addConfigColor("MatrixWidget", "mtxHeadTxColor", &m_headTextColor, t_matrixHeadTextColor, false);
    m_config.addConfigInt("MatrixWidget", "mtxLineMin", &m_lineMin, t_matrixLineMin, true);
    m_config.addConfigInt("MatrixWidget", "mtxLineMax", &m_lineMax, t_matrixLineMax, true);
    m_config.addConfigInt("MatrixWidget", "mtxSpeedMin", &m_speedMin, t_matrixSpeedMin, true);
    m_config.addConfigInt("MatrixWidget", "mtxSpeedMax", &m_speedMax, t_matrixSpeedMax, true);
    m_config.addConfigInt("MatrixWidget", "mtxUpdateInt", &m_updateInterval, t_matrixUpdateInterval, true);
}

void MatrixWidget::setup() {
    int R;
    int G;
    int B;

    ConfigManager *cm = ConfigManager::getInstance();
    bool l_bigFont = cm->getConfigBool("mtxBigFont", false);
    if (l_bigFont)
        matrix_effect.init(&m_manager, true, false);
    else
        matrix_effect.init(&m_manager);

    matrix_effect.setup(m_lineMin, m_lineMax, m_speedMin, m_speedMax, m_updateInterval);

    R = ((m_textColor >> 11) & 0x1F) * 255 / 31;
    G = ((m_textColor >> 5) & 0x3F) * 255 / 63;
    B = (m_textColor & 0x1F) * 255 / 31;
    matrix_effect.setTextColor(R, G, B);

    R = ((m_headTextColor >> 11) & 0x1F) * 255 / 31;
    G = ((m_headTextColor >> 5) & 0x3F) * 255 / 63;
    B = (m_headTextColor & 0x1F) * 255 / 31;
    matrix_effect.setHeadCharColor(R, G, B);
}

void MatrixWidget::update(bool force) {
    CrashTrace::mark("matrix:update");
}

void MatrixWidget::draw(bool force) {
    CrashTrace::mark("matrix:draw");
    m_manager.selectAllScreens();
    matrix_effect.loop();
}

void MatrixWidget::buttonPressed(uint8_t buttonId, ButtonState state) {
}

String MatrixWidget::getName() {
    return "Matrix Screen";
}
