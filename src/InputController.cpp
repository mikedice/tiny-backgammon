#include "InputController.h"

InputController* InputController::instance_ = nullptr;

InputController::InputController(uint8_t clkPin, uint8_t dtPin, uint8_t swPin)
    : encoder_(clkPin, dtPin, RotaryEncoder::LatchMode::TWO03),
      clkPin_(clkPin), dtPin_(dtPin), swPin_(swPin) {}

void InputController::isrTrampoline() {
    if (instance_) instance_->encoder_.tick();
}

void InputController::begin() {
    instance_ = this;
    pinMode(swPin_, INPUT_PULLUP);

    // RotaryEncoder's constructor already set clkPin_/dtPin_ to INPUT_PULLUP.
    attachInterrupt(digitalPinToInterrupt(clkPin_), isrTrampoline, CHANGE);
    attachInterrupt(digitalPinToInterrupt(dtPin_), isrTrampoline, CHANGE);

    lastPosition_ = encoder_.getPosition();
    rawState_ = digitalRead(swPin_);
    debouncedState_ = rawState_;
}

void InputController::poll() {
    bool raw = digitalRead(swPin_);
    unsigned long now = millis();

    if (raw != rawState_) {
        rawState_ = raw;
        lastEdgeAt_ = now;
    }

    if ((now - lastEdgeAt_) >= kDebounceMs && debouncedState_ != rawState_) {
        debouncedState_ = rawState_;
        if (debouncedState_ == LOW) {
            pressedAt_ = now;
            longPressFired_ = false;
        } else if (!longPressFired_) {
            pendingClick_ = true;
        }
    }

    if (debouncedState_ == LOW && !longPressFired_ && (now - pressedAt_) >= kLongPressMs) {
        longPressFired_ = true;
        pendingLongPress_ = true;
    }
}

int InputController::consumeRotation() {
    long pos = encoder_.getPosition();
    int delta = static_cast<int>(pos - lastPosition_);
    lastPosition_ = pos;
    return delta;
}

bool InputController::consumeClick() {
    bool v = pendingClick_;
    pendingClick_ = false;
    return v;
}

bool InputController::consumeLongPress() {
    bool v = pendingLongPress_;
    pendingLongPress_ = false;
    return v;
}
