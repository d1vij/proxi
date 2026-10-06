#pragma once

#include "AudioEngine.h"

namespace Chimes
{

const BuzzerNote startupChime[] = {
    {659, 100},  // E5
    {0, 50},     // Rest
    {659, 100},  // E5
    {0, 100},    // Rest
    {659, 100},  // E5
    {0, 100},    // Rest
    {523, 100},  // C5
    {659, 150},  // E5
    {784, 300}   // G5
};

}