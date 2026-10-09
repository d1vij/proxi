#include "MenuState.h"

#include <Arduino.h>

#include <vector>

#include "Display.h"

// Define external hardware buttons
DebouncedButton menu_btn_up(BTN_UP, debounce, INPUT_PULLUP);
DebouncedButton menu_btn_down(BTN_DOWN, debounce, INPUT_PULLUP);
DebouncedButton menu_btn_left(BTN_LEFT, debounce, INPUT_PULLUP);
DebouncedButton menu_btn_right(BTN_RIGHT, debounce, INPUT_PULLUP);

MenuState Menu;

void MenuState::set_items(const std::vector<menu_item_title_t>& items)
{
    this->items = items;
    total_items = this->items.size();
    curr_idx = 0;
    last_idx = SIZE_MAX;  // Force initial display redraw

    menu_state = MENU_STATE_ACTION::MENU_NAVIGATING;
    state_changed = true;
}

void MenuState::nextItem()
{
    if (menu_state != MENU_STATE_ACTION::MENU_NAVIGATING || total_items == 0) return;

    if (curr_idx + 1 < total_items) {
        curr_idx++;
        state_changed = true;
    }
}

void MenuState::prevItem()
{
    if (menu_state != MENU_STATE_ACTION::MENU_NAVIGATING || total_items == 0) return;

    if (curr_idx > 0) {
        curr_idx--;
        state_changed = true;
    }
}

void MenuState::selectCurrItem()
{
    if (total_items == 0) return;

    menu_state = MENU_STATE_ACTION::MENU_SELECTED;
    selected_idx = curr_idx;
    state_changed = true;
    Display::clear();
}

bool MenuState::has_updated()
{
    bool temp = state_changed;
    state_changed = false;  // Clear dirty flag after check
    return temp;
}

void MenuState::display()
{
    // Don't render if not actively navigating
    if (menu_state != MENU_STATE_ACTION::MENU_NAVIGATING) return;

    // Don't redraw if index hasn't changed
    if (last_idx == curr_idx) return;
    last_idx = curr_idx;

    if (total_items == 0) return;

    Display::clear();

    size_t top_idx = (curr_idx % 2 == 0) ? curr_idx : curr_idx - 1;

    // Line 1
    Display::setCursor(0, 0);
    if (top_idx == curr_idx) {
        Display::print(SELECTED_ITEM_MARKER);
    } else {
        Display::print(NON_SELECTED_ITEM_MARKER);
    }
    Display::print(items[top_idx]);

    // Line 2 (only if second item exists)
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