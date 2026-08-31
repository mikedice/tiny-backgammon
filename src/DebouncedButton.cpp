#include "DebouncedButton.h"

DebouncedButton::DebouncedButton(uint8_t pin) : pin_(pin) {}

void DebouncedButton::begin() {
    pinMode(pin_, INPUT_PULLUP);
    rawState_ = digitalRead(pin_);
    debouncedState_ = rawState_;
}

void DebouncedButton::poll() {
    bool raw = digitalRead(pin_);
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

bool DebouncedButton::consumeClick() {
    bool v = pendingClick_;
    pendingClick_ = false;
    return v;
}

bool DebouncedButton::consumeLongPress() {
    bool v = pendingLongPress_;
    pendingLongPress_ = false;
    return v;
}
