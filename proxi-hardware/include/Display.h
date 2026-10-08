#pragma once

#include <Printable.h>
#include <Wire.h>

#include "LiquidCrystal_I2C.h"

static const int LCD_I2C_ADDR = 0x27;

static const int LCD_ROWS = 2;
static const int LCD_COLS = 16;

// clang-format off
static const char EMPTY_LINE[] = {
    ' ', ' ', ' ', ' ',
    ' ', ' ', ' ', ' ',
    ' ', ' ', ' ', ' ',
    ' ', ' ', ' ', ' ',
    '\0'
};

// clang-format on

static void pad_string(std::string& s, size_t len = LCD_COLS)
{
    if (s.length() < len) {
        s.resize(len, ' ');
    }
}

class Display
{
   private:
    static inline LiquidCrystal_I2C lcd = LiquidCrystal_I2C(LCD_I2C_ADDR, LCD_COLS, LCD_ROWS);

   public:
    static void init()
    {
        Wire.begin();
        lcd.init();
        lcd.clear();

        lcd.home();
        lcd.display();
        lcd.backlight();
    };
    static LiquidCrystal_I2C* instance() { return &lcd; }

    static void home() { lcd.home(); }
    static void clear() { lcd.clear(); }

    // NOTE: might wanna generalize for nth row when using some other display
    static void clearFirstLine()
    {
        lcd.setCursor(0, 0);
        lcd.print(EMPTY_LINE);
    }
    static void clearSecondLine()
    {
        lcd.setCursor(0, 1);
        lcd.print(EMPTY_LINE);
    }

    static void setCursor(uint8_t col, uint8_t row) { lcd.setCursor(col, row); }

    static void print(const char* text) { lcd.print(text); }
};