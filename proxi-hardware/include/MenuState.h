#pragma once
#include <Arduino.h>

#include <vector>

#include "DebouncedButton.h"
#include "Display.h"
#include "pins.h"

static inline int debounce = 20;
static const char* SELECTED_ITEM_MARKER = ">";
static const char* NON_SELECTED_ITEM_MARKER = " ";

// Hardware buttons (instantiated in MenuState.cpp)
extern DebouncedButton menu_btn_up;
extern DebouncedButton menu_btn_down;
extern DebouncedButton menu_btn_left;
extern DebouncedButton menu_btn_right;

enum class MENU_STATE_ACTION { MENU_CLOSED, MENU_NAVIGATING, MENU_SELECTED };

using menu_item_title_t = const char*;

class MenuState
{
   private:
    std::vector<menu_item_title_t> items;
    size_t last_idx = SIZE_MAX;
    size_t selected_idx = 0;
    size_t total_items = 0;

    MENU_STATE_ACTION menu_state = MENU_STATE_ACTION::MENU_CLOSED;
    bool state_changed = false;

   public:
    size_t curr_idx = 0;
    MenuState() {}

    void set_items(const std::vector<menu_item_title_t>& items);
    void nextItem();
    void prevItem();
    void selectCurrItem();

    size_t selected() const { return selected_idx; }

    /**
     * Checks if state or selection changed, then clears dirty flag.
     */
    bool has_updated();

    void close()
    {
        menu_state = MENU_STATE_ACTION::MENU_CLOSED;
        state_changed = true;
        curr_idx = 0;
        last_idx = SIZE_MAX;
        Display::clear();
    };

    void display();

    void navigate()
    {
        menu_state = MENU_STATE_ACTION::MENU_NAVIGATING;
        state_changed = true;
        last_idx = SIZE_MAX;
    }

    MENU_STATE_ACTION state() const { return menu_state; }
};

extern MenuState Menu;