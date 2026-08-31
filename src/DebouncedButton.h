#pragma once
#include <Arduino.h>

// A single momentary push button wired to GND with INPUT_PULLUP: debounced
// state, edge-triggered short-click, and long-press detection.
class DebouncedButton {
public:
    explicit DebouncedButton(uint8_t pin);

    void begin();
    void poll(); // call every loop() iteration

    // Edge-triggered, consumed on read.
    bool consumeClick();
    bool consumeLongPress();

private:
    uint8_t pin_;

    bool rawState_ = HIGH;
    bool debouncedState_ = HIGH;
    unsigned long lastEdgeAt_ = 0;
    unsigned long pressedAt_ = 0;
    bool longPressFired_ = false;
    bool pendingClick_ = false;
    bool pendingLongPress_ = false;

    static constexpr unsigned long kDebounceMs = 15;
    static constexpr unsigned long kLongPressMs = 600;
};
