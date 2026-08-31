#pragma once
#include <Arduino.h>
#include <RotaryEncoder.h>
#include "DebouncedButton.h"

// Wraps a rotary encoder + its push button: interrupt-driven rotation
// counting, plus the button's debounced short-click vs. long-press
// detection (long-press = "back/cancel").
class InputController {
public:
    InputController(uint8_t clkPin, uint8_t dtPin, uint8_t swPin);

    void begin();
    void poll(); // call every loop() iteration

    // Net encoder steps since the last call (consumes them).
    int consumeRotation();

    // Edge-triggered, consumed on read.
    bool consumeClick();
    bool consumeLongPress();

private:
    static InputController* instance_;
    static void isrTrampoline();

    RotaryEncoder encoder_;
    uint8_t clkPin_, dtPin_;
    long lastPosition_ = 0;

    DebouncedButton button_;
};
