#ifndef BUTTON_H
#define BUTTON_H
#include <Arduino.h>
#include <freertos/portmacro.h>

enum ButtonState {
    BTN_NOTHING,
    BTN_SHORT,
    BTN_MEDIUM,
    BTN_LONG,
    BTN_VERY_LONG
};

enum Buttons {
    BUTTON_INVALID = 0,
    BUTTON_LEFT = 1,
    BUTTON_MIDDLE = 2,
    BUTTON_OK = 2, // backwards compatibility
    BUTTON_RIGHT = 3,
};

class Button {
public:
    Button();
    void begin(uint8_t pin);
    bool pressedShort();
    bool pressedMedium();
    bool pressedLong();
    bool pressedVeryLong();
    ButtonState getState();
    void isrButtonChange();
    void updateState(unsigned long currentTime);

#if BUTTON_MODE == INPUT_PULLDOWN
    const static bool PRESSED_LEVEL = HIGH;
    const static bool RELEASED_LEVEL = LOW;
#else
    const static bool PRESSED_LEVEL = LOW;
    const static bool RELEASED_LEVEL = HIGH;
#endif

#ifdef BUTTON_DEBOUNCE_TIME
    const static unsigned long DEBOUNCE_TIME = BUTTON_DEBOUNCE_TIME;
#else
    const static unsigned long DEBOUNCE_TIME = 35;
#endif
#ifdef BUTTON_MEDIUM_PRESS_TIME
    const static unsigned long MEDIUM_PRESS_TIME = BUTTON_MEDIUM_PRESS_TIME;
#else
    const static unsigned long MEDIUM_PRESS_TIME = 500;
#endif
#ifdef BUTTON_LONG_PRESS_TIME
    const static unsigned long LONG_PRESS_TIME = BUTTON_LONG_PRESS_TIME;
#else
    const static unsigned long LONG_PRESS_TIME = 2000;
#endif
#ifdef BUTTON_VERY_LONG_PRESS_TIME
    const static unsigned long VERY_LONG_PRESS_TIME = BUTTON_VERY_LONG_PRESS_TIME;
#else
    const static unsigned long VERY_LONG_PRESS_TIME = 15000; // Default 15 seconds
#endif

private:
    uint8_t m_pin;
    volatile bool m_pinLevel;
    volatile ButtonState m_state;
    volatile bool m_hasChanged;
    volatile unsigned long m_lastPinLevelChange;
    volatile unsigned long m_pressedSince;
    portMUX_TYPE m_spinlock = portMUX_INITIALIZER_UNLOCKED;

    bool has_changed();
};

#endif // BUTTON_H
