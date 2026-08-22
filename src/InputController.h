#pragma once
#include <Arduino.h>
#include <RotaryEncoder.h>

// Wraps a rotary encoder + its push button: interrupt-driven rotation
// counting, debounced button state, and short-click vs. long-press
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
    uint8_t clkPin_, dtPin_, swPin_;
    long lastPosition_ = 0;

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
