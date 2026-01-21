#include "EyesWidget.h"
#include "EyesTranslations.h"
#include "icons.h"
#include <Arduino.h>

EyesWidget::EyesWidget(ScreenManager &manager, ConfigManager &config) : Widget(manager, config) {
    m_enabled = (INCLUDE_EYES == WIDGET_ON);

    // Add configuration options
    m_config.addConfigBool("Eyes Widget", "eyesEnabled", &m_enabled, t_enableWidget);
    m_config.addConfigColor("Eyes Widget", "eyesEyeColor", &m_eyeColor, t_eyesEyeColor, false);
    m_config.addConfigColor("Eyes Widget", "eyesIrisColor", &m_irisColor, t_eyesIrisColor, false);
    m_config.addConfigColor("Eyes Widget", "eyesPupilColor", &m_pupilColor, t_eyesPupilColor, false);
    m_config.addConfigColor("Eyes Widget", "eyesEyelidColor", &m_eyelidColor, t_eyesEyelidColor, false);
    m_config.addConfigBool("Eyes Widget", "eyesShowNose", &m_noseEnabled, t_eyesShowNose, false);

    // Add interval configuration (advanced settings)
    m_config.addConfigInt("Eyes Widget", "eyesPupilMax", &m_pupilMoveMaxInterval, t_eyesPupilMoveMax, true);

    // New "Closed Eyes" configuration
    m_config.addConfigInt("Eyes Widget", "eyesLongCloseMin", &m_longCloseMinInterval, t_eyesLongCloseMin, true);
    m_config.addConfigInt("Eyes Widget", "eyesLongCloseMax", &m_longCloseMaxInterval, t_eyesLongCloseMax, true);
    m_config.addConfigInt("Eyes Widget", "eyesLongCloseDur", &m_longCloseDurationMin, t_eyesLongCloseDuration, true);
}

void EyesWidget::setup() {
    // Initialize random seed
    randomSeed(analogRead(0));

    // Set initial random delays
    m_nextPupilMoveDelay = randomInterval(m_pupilMoveMinInterval, m_pupilMoveMaxInterval);
    m_nextLongCloseDelay = randomInterval(m_longCloseMinInterval, m_longCloseMaxInterval);

    m_lastPupilMoveTime = millis();
    m_lastLongCloseTime = millis();
    m_lastBlinkFrameTime = millis();
    m_firstDraw = true;
}

void EyesWidget::update(bool force) {
    unsigned long currentTime = millis();
    bool stateChanged = false;

    // Update pupil position
    if (currentTime - m_lastPupilMoveTime >= m_nextPupilMoveDelay) {
        updatePupilPosition();
        m_lastPupilMoveTime = currentTime;
        m_nextPupilMoveDelay = randomInterval(m_pupilMoveMinInterval, m_pupilMoveMaxInterval);
        stateChanged = true;
        m_pupilJustMoved = true; // Set flag to force both eyes redraw
    }

    // Update blink state - trigger ONLY on long close timer
    if (m_blinkState == BlinkState::OPEN) {
        if (currentTime - m_lastLongCloseTime >= m_nextLongCloseDelay) {
            m_isLongClose = true;
            m_currentClosedDuration = randomInterval(m_longCloseDurationMin, m_longCloseDurationMax);
            
            m_blinkState = BlinkState::CLOSING;
            m_blinkStartTime = currentTime;
            m_lastLongCloseTime = currentTime;
            m_lastBlinkFrameTime = currentTime;
            m_nextLongCloseDelay = randomInterval(m_longCloseMinInterval, m_longCloseMaxInterval);
            stateChanged = true;
        }
    }

    // Store previous blink progress before update
    float prevBlinkProgress = m_blinkProgress;

    // Update blink animation at higher frequency for smooth movement
    if (m_blinkState != BlinkState::OPEN) {
        if (currentTime - m_lastBlinkFrameTime >= BLINK_FRAME_INTERVAL) {
            updateBlinkState();
            m_lastBlinkFrameTime = currentTime;

            // During any blink animation phase, only redraw the blinking eye
            // unless a pupil move was just triggered in this same loop iteration
            stateChanged = true;
            if (!m_pupilJustMoved) {
                m_blinkRedrawOnly = true;
            }
        }
    } else {
        updateBlinkState();
    }

    // Set redraw flag if state changed or forced
    if (stateChanged || force) {
        m_needsRedraw = true;
    }

    // Reset flags at the start of update to ensure they only persist for one draw cycle
    // (Actually, they should be set in update and reset in draw)
    if (currentTime - m_lastPupilMoveTime < 100) { // Catch the frame where it changed
        // This is a bit tricky with how update/draw are called.
        // Let's use the stateChanged from pupil move.
    }
}

void EyesWidget::draw(bool force) {
    // Only redraw if something changed or first draw
    if (!m_needsRedraw && !m_firstDraw && !force) {
        return;
    }

    int screenWidth = 240;
    int screenHeight = 240;
    int centerX = screenWidth / 2;
    int centerY = screenHeight / 2;

    // First draw: clear entire screen
    if (m_firstDraw) {
        m_manager.selectScreen(1);
        m_manager.fillScreen(TFT_BLACK);
        drawEye(1, centerX, centerY);

        m_manager.selectScreen(3);
        m_manager.fillScreen(TFT_BLACK);
        drawEye(3, centerX, centerY);

        // Draw nose on screen 2 (middle screen) if enabled, otherwise clear it
        m_manager.selectScreen(2);
        m_manager.fillScreen(TFT_BLACK);
        if (m_noseEnabled) {
            drawNose(2);
        }

        m_firstDraw = false;
    } else {
        // Incremental update: only redraw changed parts
        
        if (m_pupilJustMoved || force) {
            // Pupil moved: must redraw both eyes to ensure they are in sync
            drawEye(1, centerX, centerY);
            drawEye(3, centerX, centerY);
            m_pupilJustMoved = false;
        } else if (m_blinkProgress > 0 || m_previousBlinkProgress > 0) {
            // During blink, update both eyes
            drawEye(1, centerX, centerY);
            drawEye(3, centerX, centerY);
        } else {
            // Default: redraw both if needsRedraw is set for some other reason
            drawEye(1, centerX, centerY);
            drawEye(3, centerX, centerY);
        }
    }

    // Update previous state
    m_previousPupilPosition = m_pupilPosition;
    m_previousBlinkProgress = m_blinkProgress;
    m_needsRedraw = false;
}

void EyesWidget::buttonPressed(uint8_t buttonId, ButtonState state) {
    // No special button handling for now
}

String EyesWidget::getName() {
    return "Eyes";
}

void EyesWidget::drawEye(int screenIndex, int centerX, int centerY) {
    m_manager.selectScreen(screenIndex);

    // Choose the correct background based on screen index (Screen 1 is Left, Screen 3 is Right)
    const byte *eyeBgStart = (screenIndex == 1) ? eye_white_L_start : eye_white_R_start;
    uint32_t eyeBgSize = (screenIndex == 1) ? (eye_white_L_end - eye_white_L_start) : (eye_white_R_end - eye_white_R_start);

    // On first draw or if not initialized, draw the complete eye
    if (m_firstDraw) {
        // Draw the photorealistic eye white and surrounding skin (mirrored for left/right)
        m_manager.drawJpg(0, 0, eyeBgStart, eyeBgSize);

        // Draw iris and pupil
        int pupilOffsetX = getPupilOffsetX();
        m_manager.setTransparentColor(TFT_BLACK); // Iris JPG has black background
        m_manager.drawJpg(centerX + pupilOffsetX - IRIS_RADIUS, centerY - IRIS_RADIUS, eye_iris_start, eye_iris_end - eye_iris_start);
        m_manager.resetTransparentColor();

        // Always draw eyelid (visible even when open at 0% blink)
        drawEyelid(screenIndex, centerX, centerY, m_blinkProgress);
    } else {
        // Incremental update: only redraw what changed

        // If pupil position changed or blink changed, we redraw the relevant parts
        if (m_pupilPosition != m_previousPupilPosition || abs(m_blinkProgress - m_previousBlinkProgress) > 0.01f) {
            // Since we use JPG assets and transparency, it's safer to redraw the background
            m_manager.drawJpg(0, 0, eyeBgStart, eyeBgSize);

            int pupilOffsetX = getPupilOffsetX();
            m_manager.setTransparentColor(TFT_BLACK);
            m_manager.drawJpg(centerX + pupilOffsetX - IRIS_RADIUS, centerY - IRIS_RADIUS, eye_iris_start, eye_iris_end - eye_iris_start);
            m_manager.resetTransparentColor();

            // Draw eyelid based on blink progress
            drawEyelid(screenIndex, centerX, centerY, m_blinkProgress);
        } else {
            // Even if nothing changed, ensure eyelid is visible (at its current state)
            drawEyelid(screenIndex, centerX, centerY, m_blinkProgress);
        }
    }
}

void EyesWidget::drawPupil(int screenIndex, int centerX, int centerY, int offsetX) {
    m_manager.selectScreen(screenIndex);
    m_manager.fillCircle(centerX + offsetX, centerY, PUPIL_RADIUS, m_pupilColor);
}

void EyesWidget::drawNose(int screenIndex) {
    m_manager.selectScreen(screenIndex);
    m_manager.drawJpg(0, 0, eye_nose_start, eye_nose_end - eye_nose_start);
}

void EyesWidget::drawEyelid(int screenIndex, int centerX, int centerY, float closePercent) {
    m_manager.selectScreen(screenIndex);

    // Apply easing for more natural movement
    float easedPercent = easeInOutQuad(closePercent);

    // Calculate vertical position for the eyelid JPG
    // When open (0%), it should be just off-screen or at the very top
    // When closed (100%), it should be fully centered
    int eyelidHeight = 240;
    int yOffset = -eyelidHeight + (int)(eyelidHeight * easedPercent);
    
    // Always show or hide based on offset
    // User wants no eyelid portion visible when open
    // if (yOffset < -220) yOffset = -220; 

    const byte *eyelidStart = (screenIndex == 1) ? eye_eyelid_start : eye_eyelid_mirrored_start;
    uint32_t eyelidSize = (screenIndex == 1) ? (eye_eyelid_end - eye_eyelid_start) : (eye_eyelid_mirrored_end - eye_eyelid_mirrored_start);

    // Draw the photorealistic eyelid JPG sliding down
    // We use transparency (black) so it overlaps nicely with the eye background if needed
    // However, the eyelid image itself usually covers the whole width
    m_manager.setTransparentColor(TFT_BLACK);
    m_manager.drawJpg(0, yOffset, eyelidStart, eyelidSize);
    m_manager.resetTransparentColor();
}

void EyesWidget::updatePupilPosition() {
    // Randomly choose a new pupil position
    int randVal = random(3);
    switch (randVal) {
    case 0:
        m_pupilPosition = PupilPosition::LEFT;
        break;
    case 1:
        m_pupilPosition = PupilPosition::CENTER;
        break;
    case 2:
        m_pupilPosition = PupilPosition::RIGHT;
        break;
    }
}

void EyesWidget::updateBlinkState() {
    if (m_blinkState == BlinkState::OPEN) {
        m_blinkProgress = 0.0f;
        return;
    }

    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - m_blinkStartTime;

    switch (m_blinkState) {
    case BlinkState::CLOSING:
        m_blinkProgress = (float) elapsed / BLINK_CLOSE_DURATION;
        if (m_blinkProgress >= 1.0f) {
            m_blinkProgress = 1.0f;
            m_blinkState = BlinkState::CLOSED;
            m_blinkStartTime = currentTime;
        }
        break;

    case BlinkState::CLOSED:
        m_blinkProgress = 1.0f;
        if (elapsed >= m_currentClosedDuration) {
            m_blinkState = BlinkState::OPENING;
            m_blinkStartTime = currentTime;
        }
        break;

    case BlinkState::OPENING:
        m_blinkProgress = 1.0f - ((float) elapsed / BLINK_OPEN_DURATION);
        if (m_blinkProgress <= 0.0f) {
            m_blinkProgress = 0.0f;
            m_blinkState = BlinkState::OPEN;
        }
        break;

    case BlinkState::OPEN:
        // Already handled above
        break;
    }
}

int EyesWidget::getPupilOffsetX() {
    switch (m_pupilPosition) {
    case PupilPosition::LEFT:
        return -PUPIL_MOVE_RANGE;
    case PupilPosition::CENTER:
        return 0;
    case PupilPosition::RIGHT:
        return PUPIL_MOVE_RANGE;
    default:
        return 0;
    }
}

unsigned long EyesWidget::randomInterval(int minVal, int maxVal) {
    return random(minVal, maxVal + 1);
}

// Easing function for smooth, natural animations
float EyesWidget::easeInOutQuad(float t) {
    // Quadratic ease in/out for smooth acceleration and deceleration
    if (t < 0.5f) {
        return 2.0f * t * t;
    } else {
        t = t - 1.0f;
        return 1.0f - 2.0f * t * t;
    }
}
