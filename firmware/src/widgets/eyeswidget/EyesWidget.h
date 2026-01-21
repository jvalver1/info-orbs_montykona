#ifndef EYESWIDGET_H
#define EYESWIDGET_H

#include "Widget.h"
#include "config_helper.h"
#include <TFT_eSPI.h>

// Animation states for blinking
enum class BlinkState {
    OPEN,
    CLOSING,
    CLOSED,
    OPENING
};

// Pupil positions
enum class PupilPosition {
    LEFT,
    CENTER,
    RIGHT
};

class EyesWidget : public Widget {
public:
    EyesWidget(ScreenManager &manager, ConfigManager &config);
    void setup() override;
    void update(bool force = false) override;
    void draw(bool force = false) override;
    void buttonPressed(uint8_t buttonId, ButtonState state) override;
    String getName() override;

private:
    // Drawing methods
    void drawEye(int screenIndex, int centerX, int centerY);
    void drawPupil(int screenIndex, int centerX, int centerY, int offsetX);
    void drawEyelid(int screenIndex, int centerX, int centerY, float closePercent);
    void drawNose(int screenIndex);

    // Animation update methods
    void updatePupilPosition();
    void updateBlinkState();

    // Configuration parameters
    int m_eyeColor = TFT_WHITE; // Sclera (white of eye)
    int m_irisColor = 0x4A69; // Blue-ish iris
    int m_pupilColor = TFT_BLACK; // Pupil
    int m_eyelidColor = 0xFDC0; // Skin tone
    int m_noseColor = 0xFDC0; // Nose color (same as eyelid - skin tone)
    int m_noseOutlineColor = 0xD69A; // Slightly darker for definition

    // Eye dimensions
    const int EYE_RADIUS = 90; // Outer eye radius
    const int IRIS_RADIUS = 40; // Iris radius
    const int PUPIL_RADIUS = 20; // Pupil radius
    const int PUPIL_MOVE_RANGE = 20; // How far pupil can move

    // Animation state
    PupilPosition m_pupilPosition = PupilPosition::CENTER;
    BlinkState m_blinkState = BlinkState::OPEN;
    float m_blinkProgress = 0.0f; // 0.0 = open, 1.0 = closed

    // Timing
    unsigned long m_lastPupilMoveTime = 0;
    unsigned long m_nextPupilMoveDelay = 3000; // Time until next pupil move
    unsigned long m_lastBlinkTime = 0;
    unsigned long m_nextBlinkDelay = 4000; // Time until next blink
    unsigned long m_blinkStartTime = 0;

    // Animation intervals (configurable)
    int m_pupilMoveMinInterval = 2000; // Min time between pupil moves
    int m_pupilMoveMaxInterval = 4000; // Max time between pupil moves
    int m_blinkMinInterval = 5000; // Min time between blinks (was 3000)
    int m_blinkMaxInterval = 10000; // Max time between blinks (was 6000)

    // Blink animation timing - slower for buttery smooth display refresh (~430ms total)
    const int BLINK_CLOSE_DURATION = 160; // Time to close eyelid (ms)
    const int BLINK_CLOSED_DURATION = 50; // Time eyelid stays closed (ms)
    const int BLINK_OPEN_DURATION = 220; // Time to open eyelid (ms) - slowest phase
    const int BLINK_FRAME_INTERVAL = 20; // Update every 20ms (50 fps) - very smooth

    // Helper methods
    int getPupilOffsetX();
    unsigned long randomInterval(int minVal, int maxVal);
    float easeInOutQuad(float t); // Easing function for smooth animation

    // State tracking for efficient rendering
    PupilPosition m_previousPupilPosition = PupilPosition::CENTER;
    float m_previousBlinkProgress = 0.0f;
    bool m_needsRedraw = true; // Flag to trigger redraw
    bool m_firstDraw = true; // First draw needs full init
    unsigned long m_lastBlinkFrameTime = 0; // For smooth blink animation timing
};

#endif // EYESWIDGET_H
