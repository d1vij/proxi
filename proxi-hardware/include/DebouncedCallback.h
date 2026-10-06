#pragma once
#include <Arduino.h>

#include <functional>

typedef std::function<void()> void_fn;

/**
 * wrapper class for a debounced callback effect.
 * once wrapped, the function wrapper is continously polled
 * in a while loop by calling the  call() method
 */
class DebouncedCallback
{
   private:
    void_fn callback;
    unsigned long debounceMs;
    unsigned long lastRun = 0;

   public:
    DebouncedCallback(const void_fn callback, unsigned long debounceMs)
        : callback(callback), debounceMs(debounceMs)
    {
    }

    inline void call()
    {
        unsigned long now = millis();

        if (now - this->lastRun > this->debounceMs) {
            this->lastRun = now;
            this->callback();
        }
    }
};
