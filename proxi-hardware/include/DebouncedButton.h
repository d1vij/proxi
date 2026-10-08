#pragma once
#include <Arduino.h>

class DebouncedButton {
private:
    uint8_t pin;
    unsigned long debounceMs;
    bool pullup;
    bool lastReading = false;
    bool debouncedState = false;
    unsigned long lastDebounceTime = 0;

public:
    DebouncedButton(uint8_t pin, unsigned long debounceMs = 50, int mode = INPUT_PULLUP);
    bool isPressed();
};