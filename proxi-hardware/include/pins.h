#pragma once

#include <Arduino.h>

#include "platform.h"

#if IS_ARDUINO

const int BUZZER = 5;

const int BTN_RIGHT = A0;
const int BTN_DOWN = A1;
const int BTN_UP = A2;
const int BTN_LEFT = A3;

const int RC522_RST = 2;  // Reset
const int RC522_SS = 10;  // Slave Select / CS (Moved off D3 for SPI standard)

#else

/*
    INFO: ESP32 GPIO Pin Mapping
*/

const int BUZZER = 25;

const int BTN_RIGHT = 4;
const int BTN_UP = 19;
const int BTN_DOWN = 5;
const int BTN_LEFT = 18;

#endif