#include "DebouncedButton.h"

#include <Arduino.h>

DebouncedButton::DebouncedButton(uint8_t pin, unsigned long debounceMs, int mode)
    : pin(pin), debounceMs(debounceMs)
{
    this->pullup = (mode == INPUT_PULLUP);
}

void DebouncedButton::begin()
{
    pinMode(this->pin, this->pullup ? INPUT_PULLUP : INPUT);
    this->lastState = digitalRead(this->pin);
    this->debouncedState = this->lastState;
}

bool DebouncedButton::isPressed()
{
    bool reading = digitalRead(this->pin);
    unsigned long now = millis();

    if (reading != this->lastState) {
        this->lastDebounceTime = now;
        this->lastState = reading;
    }

    if ((now - this->lastDebounceTime) > this->debounceMs) {
        if (reading != this->debouncedState) {
            this->debouncedState = reading;

            // invert active state when the input is PULLUP
            bool activeState = this->pullup ? LOW : HIGH;

            if (this->debouncedState == activeState) {
                return true;
            }
        }
    }

    return false;
}