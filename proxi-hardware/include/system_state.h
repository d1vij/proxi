#pragma once

#include <string>

#include "Display.h"
#include "MenuState.h"

enum SYSTEM_STATES { FINE, ERROR };
static SYSTEM_STATES SYSTEM_STATE = FINE;
static std::string SYSTEM_ERROR = "";

static void unrecov_system_error(std::string msg)
{
    Display::clear();
    Display::home();
    Display::print("unrecov sys err");
    Serial.println("unrecoverable system error");
    Serial.println(msg.c_str());
    Serial.println("halting proxi");
    while(true);
}

static void system_error(std::string msg)
{
    SYSTEM_STATE = ERROR;
    SYSTEM_ERROR = std::move(msg);
}

static void ack_error()
{
    SYSTEM_STATE = FINE;
    SYSTEM_ERROR = "";
}

static inline bool is_system_fine() { return SYSTEM_STATE == FINE; }

static inline void state_check()
{
    if (!is_system_fine()) {
        Display::clear();
        Display::home();
        Display::print("error: ");
        Display::print(SYSTEM_ERROR.substr(0, 9).c_str());
        Display::setCursor(0, 1);
        Display::print(SYSTEM_ERROR.substr(9, 25).c_str());
    }

    // force user to ack the error by pressing any button
    while (!is_system_fine()) {
        if (menu_btn_down.isPressed() || menu_btn_up.isPressed() || menu_btn_left.isPressed() ||
            menu_btn_right.isPressed()) {
            ack_error();
            Display::clear();
            break;
        }
    }
}