#include "DebouncedButton.h"

DebouncedButton::DebouncedButton(uint8_t pin, unsigned long debounceMs, int mode)
    : pin(pin), debounceMs(debounceMs)
{
    this->pullup = (mode == INPUT_PULLUP);
    pinMode(pin, mode);

    bool initial = digitalRead(pin);
    this->lastReading = initial;
    this->debouncedState = initial;
}

bool DebouncedButton::isPressed()
{
    bool reading = digitalRead(this->pin);
    unsigned long now = millis();

    // Reset debounce timer if physical reading changed
    if (reading != this->lastReading) {
        this->lastDebounceTime = now;
        this->lastReading = reading;
    }

    // Check if reading has stayed stable long enough
    if ((now - this->lastDebounceTime) > this->debounceMs) {
        if (reading != this->debouncedState) {
            this->debouncedState = reading;

            bool activeState = this->pullup ? LOW : HIGH;
            // Return true ONLY on the transition to active state
            if (this->debouncedState == activeState) {
                return true;
            }
        }
    }

    return false;
}