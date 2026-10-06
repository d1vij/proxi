#pragma once
#include <Arduino.h>

#include <cstdint>

/**
 * wrapper for debounced button presses.
 * isPressed() returns true only when the button state changes
 * from LOW to HIGH after a period of debounceMs milliseconds
 */
class DebouncedButton
{
   public:
    const uint8_t pin;
    const unsigned long debounceMs;
    DebouncedButton(uint8_t pin, unsigned long debounceMs, PinMode mode = INPUT);

    bool isPressed();
    void begin();

   private:
    bool lastState = false;
    unsigned long lastDebounceTime = 0;
    bool debouncedState = false;
    bool pullup;
};