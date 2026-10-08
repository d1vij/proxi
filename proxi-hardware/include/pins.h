#pragma once

#include <Arduino.h>

#include "platform.h"

#if IS_ARDUINO

const int BUZZER = D8;

const int BTN_RIGHT = D12;
const int BTN_UP = D11;
const int BTN_DOWN = D10;
const int BTN_LEFT = D9;

#else

/*
    INFO: for esp32 the gpio pin number is equal to the one displayed
    on board D19 on board would be 19 in code
*/

const int BUZZER = 20;

#endif
