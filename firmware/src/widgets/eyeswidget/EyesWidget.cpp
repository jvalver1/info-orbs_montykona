#include "EyesWidget.h"
#include "EyesTranslations.h"
#include <Arduino.h>

EyesWidget::EyesWidget(ScreenManager &manager, ConfigManager &config) : Widget(manager, config) {
    m_enabled = (INCLUDE_EYES == WIDGET_ON);

    // Add configuration options
    m_config.addConfigBool("Eyes Widget", "eyesEnabled", &m_enabled, t_enableWidget);
    m_config.addConfigColor("Eyes Widget", "eyesEyeColor", &m_eyeColor, t_eyesEyeColor, false);
    m_config.addConfigColor("Eyes Widget", "eyesIrisColor", &m_irisColor, t_eyesIrisColor, false);
    m_config.addConfigColor("Eyes Widget", "eyesPupilColor", &m_pupilColor, t_eyesPupilColor, false);
    m_config.addConfigColor("Eyes Widget", "eyesEyelidColor", &m_eyelidColor, t_eyesEyelidColor, false);

    // Add interval configuration (advanced settings)
    m_config.addConfigInt("Eyes Widget", "eyesBlinkMin", &m_blinkMinInterval, t_eyesBlinkMin, true);
    m_config.addConfigInt("Eyes Widget", "eyesBlinkMax", &m_blinkMaxInterval, t_eyesBlinkMax, true);
    m_config.addConfigInt("Eyes Widget", "eyesPupilMin", &m_pupilMoveMinInterval, t_eyesPupilMoveMin, true);
    m_config.addConfigInt("Eyes Widget", "eyesPupilMax", &m_pupilMoveMaxInterval, t_eyesPupilMoveMax, true);
}

void EyesWidget::setup() {
    // Initialize random seed
    randomSeed(analogRead(0));

    // Set initial random delays
    m_nextPupilMoveDelay = randomInterval(m_pupilMoveMinInterval, m_pupilMoveMaxInterval);
    m_nextBlinkDelay = randomInterval(m_blinkMinInterval, m_blinkMaxInterval);

    m_lastPupilMoveTime = millis();
    m_lastBlinkTime = millis();
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
    }

    // Update blink state
    if (currentTime - m_lastBlinkTime >= m_nextBlinkDelay) {
        if (m_blinkState == BlinkState::OPEN) {
            // Start a new blink
            m_blinkState = BlinkState::CLOSING;
            m_blinkStartTime = currentTime;
            m_lastBlinkTime = currentTime;
            m_lastBlinkFrameTime = currentTime;
            m_nextBlinkDelay = randomInterval(m_blinkMinInterval, m_blinkMaxInterval);
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

            // Check if blink animation is active (progress changed)
            if (abs(m_blinkProgress - prevBlinkProgress) > 0.001f) {
                stateChanged = true;
            }
        }
    } else {
        updateBlinkState();
    }

    // Set redraw flag if state changed or forced
    if (stateChanged || force) {
        m_needsRedraw = true;
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

        // Draw nose on screen 3 (middle screen - index 2)
        m_manager.selectScreen(2);
        m_manager.fillScreen(TFT_BLACK);
        drawNose(2);

        m_firstDraw = false;
    } else {
        // Incremental update: only redraw changed parts
        drawEye(1, centerX, centerY);
        drawEye(3, centerX, centerY);
        // Nose doesn't animate, so no need to redraw
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

    // On first draw or if not initialized, draw the complete eye
    if (m_firstDraw) {
        // Draw the eye white (sclera)
        m_manager.fillCircle(centerX, centerY, EYE_RADIUS, m_eyeColor);
        m_manager.drawCircle(centerX, centerY, EYE_RADIUS, TFT_BLACK);

        // Get pupil offset based on current position
        int pupilOffsetX = getPupilOffsetX();

        // Draw iris
        m_manager.fillCircle(centerX + pupilOffsetX, centerY, IRIS_RADIUS, m_irisColor);

        // Draw pupil
        drawPupil(screenIndex, centerX, centerY, pupilOffsetX);

        // Always draw eyelid (visible even when open at 0% blink)
        drawEyelid(screenIndex, centerX, centerY, m_blinkProgress);
    } else {
        // Incremental update: only redraw what changed

        // If pupil position changed, erase old and draw new
        if (m_pupilPosition != m_previousPupilPosition) {
            int oldPupilOffsetX = (m_previousPupilPosition == PupilPosition::LEFT) ? -PUPIL_MOVE_RANGE : (m_previousPupilPosition == PupilPosition::RIGHT) ? PUPIL_MOVE_RANGE
                                                                                                                                                           : 0;
            int newPupilOffsetX = getPupilOffsetX();

            // Erase old iris/pupil by drawing eye color
            m_manager.fillCircle(centerX + oldPupilOffsetX, centerY, IRIS_RADIUS, m_eyeColor);

            // Draw new iris
            m_manager.fillCircle(centerX + newPupilOffsetX, centerY, IRIS_RADIUS, m_irisColor);

            // Draw new pupil
            drawPupil(screenIndex, centerX, centerY, newPupilOffsetX);
        }

        // Handle eyelid animation (need to redraw entire eye area for smooth animation)
        if (abs(m_blinkProgress - m_previousBlinkProgress) > 0.01f) {
            // If we're blinking, we need to redraw the eye
            // First, redraw the base eye without eyelid
            int pupilOffsetX = getPupilOffsetX();

            // Only redraw if opening from a blink (erase eyelid area)
            if (m_blinkProgress < m_previousBlinkProgress || m_blinkProgress > 0.01f) {
                // Redraw eye background
                m_manager.fillCircle(centerX, centerY, EYE_RADIUS, m_eyeColor);
                m_manager.drawCircle(centerX, centerY, EYE_RADIUS, TFT_BLACK);

                // Redraw iris and pupil
                m_manager.fillCircle(centerX + pupilOffsetX, centerY, IRIS_RADIUS, m_irisColor);
                drawPupil(screenIndex, centerX, centerY, pupilOffsetX);
            }

            // Always draw eyelid overlay (even at 0% for visible top lid)
            drawEyelid(screenIndex, centerX, centerY, m_blinkProgress);
        } else {
            // Even if blink didn't change, always show eyelid
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

    int screenWidth = 240;
    int screenHeight = 240;
    int centerX = screenWidth / 2; // 120

    // Nose dimensions - rounded bulbous shape (max 80 pixels tall)
    int noseHeight = 75;
    int noseWidth = 105; // Maximum width at widest point (increased for cartoonish look)
    int noseY = screenHeight - noseHeight - 15; // Position from bottom

    // Base skin tone colors
    int baseColor = 0xFDC0; // Main skin tone
    int shadowColor = 0xE48A; // Darker for right side shadow
    int highlightColor = 0xFEE0; // Lighter for left highlight
    int outlineColor = 0x8221; // Dark brown outline

    // Draw the bulbous shape using horizontal lines with varying widths
    // The shape is wider in the middle-bottom and narrower at top
    for (int y = 0; y < noseHeight; y++) {
        float progress = (float) y / noseHeight;

        // Create bulbous shape with rounder edges
        float widthFactor;
        if (progress < 0.65f) {
            // Expanding from top to middle - smoother curve
            widthFactor = progress / 0.65f; // 0 to 1
            widthFactor = widthFactor * widthFactor * (3.0f - 2.0f * widthFactor); // Smoothstep for rounder edges
        } else {
            // Contracting from middle to bottom - gentler taper
            float t = (progress - 0.65f) / 0.35f; // 0 to 1
            widthFactor = 1.0f - (t * t * 0.25f); // Stay wide at bottom, quadratic falloff
        }

        int halfWidth = (int) (noseWidth * widthFactor / 2);

        if (halfWidth > 0) {
            int currentY = noseY + y;

            // Draw with gradient: highlight on left, shadow on right
            for (int x = -halfWidth; x <= halfWidth; x++) {
                int color;
                float xProgress = (float) x / halfWidth; // -1 to 1

                if (xProgress < -0.3f) {
                    // Left side - highlight
                    color = highlightColor;
                } else if (xProgress > 0.4f) {
                    // Right side - shadow
                    color = shadowColor;
                } else {
                    // Middle - base color
                    color = baseColor;
                }

                m_manager.drawLine(centerX + x, currentY, centerX + x, currentY, color);
            }

            // Draw dark outline on edges
            m_manager.drawLine(centerX - halfWidth, currentY, centerX - halfWidth, currentY, outlineColor);
            m_manager.drawLine(centerX + halfWidth, currentY, centerX + halfWidth, currentY, outlineColor);
        }
    }

    // Draw top and bottom outline curves
    // Top outline
    for (int x = -15; x <= 15; x++) {
        int y = noseY + (int) (2.0f * sqrt(abs(15 - abs(x))));
        if (y < noseY + 10) {
            m_manager.drawLine(centerX + x, y, centerX + x, y, outlineColor);
        }
    }

    // Draw nostrils (dark brown ovals) - larger for cartoonish effect
    int nostrilY = noseY + noseHeight - 12; // 12 pixels from bottom
    int nostrilSpacing = 20; // Distance from center (wider spacing)

    // Left nostril - draw as filled ellipse using circles (larger)
    for (int dy = -8; dy <= 8; dy++) {
        int width = (int) (6.0f * sqrt(64 - dy * dy) / 8.0f);
        for (int dx = -width; dx <= width; dx++) {
            m_manager.drawLine(centerX - nostrilSpacing + dx, nostrilY + dy,
                               centerX - nostrilSpacing + dx, nostrilY + dy, outlineColor);
        }
    }

    // Right nostril (larger)
    for (int dy = -8; dy <= 8; dy++) {
        int width = (int) (6.0f * sqrt(64 - dy * dy) / 8.0f);
        for (int dx = -width; dx <= width; dx++) {
            m_manager.drawLine(centerX + nostrilSpacing + dx, nostrilY + dy,
                               centerX + nostrilSpacing + dx, nostrilY + dy, outlineColor);
        }
    }
}

void EyesWidget::drawEyelid(int screenIndex, int centerX, int centerY, float closePercent) {
    m_manager.selectScreen(screenIndex);

    // Apply easing for more natural movement
    float easedPercent = easeInOutQuad(closePercent);

    // Calculate how much of the eye to cover
    // When fully open, show a small eyelid curve at top (8% of eye)
    // When fully closed, cover entire eye
    float minEyelidVisible = 0.08f; // Always show 8% eyelid at top
    float actualClosePercent = minEyelidVisible + (easedPercent * (1.0f - minEyelidVisible));

    int coverHeight = (int) (EYE_RADIUS * 2 * actualClosePercent);

    // Draw top eyelid
    if (coverHeight > 0) {
        int topY = centerY - EYE_RADIUS;

        // Draw eyelid with slight curve for more realistic look
        for (int i = 0; i <= coverHeight; i++) {
            int y = topY + i;
            int dy = y - centerY;

            if (abs(dy) <= EYE_RADIUS) {
                // Base width from circle equation
                int dx = (int) sqrt(EYE_RADIUS * EYE_RADIUS - dy * dy);

                // Add slight curve to eyelid edge for more natural look
                // The curve is more pronounced near the edges
                float normalizedI = (float) i / coverHeight;
                int curveAdjust = (int) (2.0f * sin(normalizedI * 3.14159f));

                m_manager.drawLine(centerX - dx, y, centerX + dx, y, m_eyelidColor);

                // Draw eyelid outline at the edge for definition
                if (i == coverHeight && coverHeight < EYE_RADIUS * 2) {
                    // Slightly darker outline for eyelid edge
                    int outlineColor = 0xD69A; // Slightly darker skin tone
                    m_manager.drawLine(centerX - dx + 2, y, centerX + dx - 2, y, outlineColor);
                }
            }
        }
    }

    // Draw bottom eyelid (only when blinking significantly)
    if (easedPercent > 0.3f) { // Only show bottom lid when more than 30% closed
        int bottomCoverHeight = (int) (coverHeight * 0.6f); // Bottom lid moves less

        for (int i = 0; i <= bottomCoverHeight; i++) {
            int y = centerY + EYE_RADIUS - i;
            int dy = y - centerY;

            if (abs(dy) <= EYE_RADIUS) {
                int dx = (int) sqrt(EYE_RADIUS * EYE_RADIUS - dy * dy);
                m_manager.drawLine(centerX - dx, y, centerX + dx, y, m_eyelidColor);
            }
        }
    }
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
        if (elapsed >= BLINK_CLOSED_DURATION) {
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
