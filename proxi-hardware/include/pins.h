#pragma once

#include <Arduino.h>

#include "platform.h"

#if IS_ARDUINO

const int LED = D10;
const int BUZZER = D9;

#else

/*
    INFO: for esp32 the gpio pin number is equal to the one displayed
    on board D19 on board would be 19 in code
*/

const int LED = 19;

#endif
