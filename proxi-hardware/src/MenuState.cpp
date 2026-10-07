#include "MenuState.h"

#include <Arduino.h>

#include <vector>

void MenuState::display(std::vector<menu_item_title_t> items)
{
    this->items = items;

    // go to top item
    curr_idx = 0;
};

MENU_STATE_ACTION MenuState::state() { return this->menu_state; }

// singleton instance for the whole menu object
MenuState Menu;