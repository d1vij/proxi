#pragma once
#include <Arduino.h>

#include <vector>

typedef enum MENU_STATE_ACTION { MENU_BACK, MENU_NAVIGATING, MENU_CHOOSEN };

using menu_item_title_t = const char*;

class MenuState
{
   private:
    std::vector<menu_item_title_t> items;
    bool enabled;
    size_t curr_idx;
    MENU_STATE_ACTION menu_state;

   public:
    MenuState() {}
    void enable() { this->enabled = true; };
    void disable() { this->enabled = false; };

    void display(std::vector<menu_item_title_t> items);

    MENU_STATE_ACTION state();
};

extern MenuState Menu;