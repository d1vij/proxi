#include "MenuState.h"

#include <Arduino.h>

#include <vector>

#include "Display.h"

void MenuState::set_items(std::vector<menu_item_title_t> items)
{
    last_menu_state = menu_state;
    menu_state = MENU_STATE_ACTION::MENU_NAVIGATING;
    this->items = items;
    total_items = this->items.size();
    curr_idx = 0;
    last_idx = SIZE_MAX;  // Force initial redrawn
}

void MenuState::nextItem()
{
    if (menu_state != MENU_STATE_ACTION::MENU_NAVIGATING) return;
    if (curr_idx + 1 < total_items) {
        curr_idx++;
    }
}

void MenuState::prevItem()
{
    if (menu_state != MENU_STATE_ACTION::MENU_NAVIGATING) return;
    if (curr_idx > 0) {
        curr_idx--;
    }
}

void MenuState::selectCurrItem()
{
    menu_state = MENU_STATE_ACTION::MENU_SELECTED;
    selected_idx = curr_idx;
    Display::clear();
}

void MenuState::display()
{
    // Don't redraw if index hasn't changed
    if (last_idx == curr_idx) return;
    last_idx = curr_idx;

    last_menu_state = menu_state;
    menu_state = MENU_STATE_ACTION::MENU_NAVIGATING;

    if (total_items == 0) return;

    Display::clear();

    size_t top_idx = (curr_idx % 2 == 0) ? curr_idx : curr_idx - 1;

    // line 1
    Display::setCursor(0, 0);
    if (top_idx == curr_idx) {
        Display::print(SELECTED_ITEM_MARKER);
    } else {
        Display::print(NON_SELECTED_ITEM_MARKER);
    }
    Display::print(items[top_idx]);

    // line 2 (only if second item exists)
    if (top_idx + 1 < total_items) {
        Display::setCursor(0, 1);
        if (top_idx + 1 == curr_idx) {
            Display::print(SELECTED_ITEM_MARKER);
        } else {
            Display::print(NON_SELECTED_ITEM_MARKER);
        }
        Display::print(items[top_idx + 1]);
    }
}


void MenuState::close() 
{

}


MENU_STATE_ACTION MenuState::state() { return this->menu_state; }

MenuState Menu;