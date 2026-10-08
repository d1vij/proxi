#pragma once
#include <Arduino.h>

#include <vector>

#include "DebouncedButton.h"
#include "pins.h"

static inline int debounce = 20;
static const char* SELECTED_ITEM_MARKER = ">";
static const char* NON_SELECTED_ITEM_MARKER = " ";

static DebouncedButton menu_btn_up(BTN_UP, debounce, INPUT_PULLUP);
static DebouncedButton menu_btn_down(BTN_DOWN, debounce, INPUT_PULLUP);
static DebouncedButton menu_btn_left(BTN_LEFT, debounce, INPUT_PULLUP);
static DebouncedButton menu_btn_right(BTN_RIGHT, debounce, INPUT_PULLUP);

typedef enum MENU_STATE_ACTION { MENU_NAVIGATING, MENU_SELECTED, MENU_IDLE };

using menu_item_title_t = const char*;

class MenuState
{
   private:
    std::vector<menu_item_title_t> items;
    size_t curr_idx;
    size_t last_idx;
    size_t selected_idx;
    size_t total_items;
    MENU_STATE_ACTION menu_state = MENU_IDLE;
    MENU_STATE_ACTION last_menu_state = MENU_IDLE;

   public:
    MenuState() {}
    void set_items(std::vector<menu_item_title_t> items);
    void nextItem();
    void prevItem();
    void selectCurrItem();
    size_t selected() { return selected_idx; };
    bool has_updated() { return !(menu_state == last_menu_state); }
    void close();

    /**
     * displays the menu
     */
    void display();

    MENU_STATE_ACTION state();
};

extern MenuState Menu;