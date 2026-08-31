#include "InputController.h"

InputController* InputController::instance_ = nullptr;

InputController::InputController(uint8_t clkPin, uint8_t dtPin, uint8_t swPin)
    // FOUR3 matches standard 4-step-per-detent mechanical encoders (the
    // common cheap kind) — one count per physical click. TWO03 was a poor
    // early guess that made rotation feel jumpy/imprecise.
    : encoder_(clkPin, dtPin, RotaryEncoder::LatchMode::FOUR3),
      clkPin_(clkPin), dtPin_(dtPin), button_(swPin) {}

void InputController::isrTrampoline() {
    if (instance_) instance_->encoder_.tick();
}

void InputController::begin() {
    instance_ = this;
    button_.begin();

    // RotaryEncoder's constructor already set clkPin_/dtPin_ to INPUT_PULLUP.
    attachInterrupt(digitalPinToInterrupt(clkPin_), isrTrampoline, CHANGE);
    attachInterrupt(digitalPinToInterrupt(dtPin_), isrTrampoline, CHANGE);

    lastPosition_ = encoder_.getPosition();
}

void InputController::poll() {
    button_.poll();
}

int InputController::consumeRotation() {
    long pos = encoder_.getPosition();
    int delta = static_cast<int>(pos - lastPosition_);
    lastPosition_ = pos;
    return delta;
}

bool InputController::consumeClick() {
    return button_.consumeClick();
}

bool InputController::consumeLongPress() {
    return button_.consumeLongPress();
}
