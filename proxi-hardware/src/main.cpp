#include <Arduino.h>
#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>
#include <ArduinoMqttClient.h>

#include <cctype>
#include <iterator>
#include <string>
#include <vector>

#include "MqttClient.h"

#if IS_ARDUINO
#include <WiFiS3.h>
#else
#include <WiFi.h>
#endif

#include <AudioEngine.h>

#include "DebouncedButton.h"
#include "DebouncedCallback.h"
#include "Display.h"
#include "MenuState.h"
#include "ProxiFriends.h"
#include "ProxiID.h"
#include "ProxiMessages.h"
#include "audio_samples.h"
#include "connectivity.h"
#include "pins.h"
#include "platform.h"
#include "secrets.h"
#include "system_state.h"

// ---
ProxiMessages Messages;
// MqttClient* mqtt = nullptr;

inline void print_board_info()
{
#if IS_ARDUINO
    Serial.println("proxi running on Arduino UNO R4 WiFi");
    Serial.println(get_proxi_id().str.c_str());
#elif IS_ESP
    Serial.println("proxi running on ESP-WROOM-32");
    Serial.println(get_proxi_id().str.c_str());
#else
#error "which board are u using bro ??"
#endif
}

// Always returns something displayable, never skips a message
static std::string sender_display_name(const std::string& sender_id)
{
    std::string name;

    if (Friends.get_name(sender_id, name) && !name.empty()) return name;

    // retry with lowercase in case of hex-case mismatch
    std::string lower = sender_id;
    for (auto& c : lower) c = static_cast<char>(tolower(c));
    if (Friends.get_name(lower, name) && !name.empty()) return name;

    // fallback: short ID so it's obvious who it is
    return sender_id.substr(0, 6);
}

// Rebuilds the message preview list from the current inbox
static void build_message_previews(std::vector<std::string>& strings,
                                   std::vector<const char*>& items)
{
    strings.clear();
    items.clear();

    for (const auto& msg : Messages.messages) {
        std::string friend_name = sender_display_name(msg.second.sender_id);
        if (friend_name.size() > 8) friend_name.resize(8);  // keep room for the preview

        int available_len = 16 - (static_cast<int>(friend_name.size()) + 1);
        if (available_len < 0) available_len = 0;

        strings.push_back(friend_name + "-" + msg.second.content.substr(0, available_len));
    }

    for (const auto& str : strings) {
        items.push_back(str.c_str());
    }
}

void home_menu()
{
    Display::clear();
    Display::home();
    Display::print("Hello ");

    Display::setCursor(0, 1);
    Display::print(std::to_string(Messages.messages.size()).c_str());
    Display::print(" messages.");
}

void render_full_message(size_t message_idx)
{
    if (message_idx >= Messages.messages.size()) return;

    auto it = Messages.messages.begin();
    std::advance(it, message_idx);

    std::string friend_name = sender_display_name(it->second.sender_id);

    Display::clear();
    Display::setCursor(0, 0);
    Display::print(friend_name.c_str());

    Display::setCursor(0, 1);
    Display::print(it->second.content.c_str());
}

void init_proxi()
{
    Serial.begin(115200);
    print_board_info();

    Display::init();
    Display::home();
    Display::print("setting up proxi");

    init_wifi();

    std::string msg;
    const char* msg_c_str;

    msg = "fetching friends";
    msg_c_str = msg.c_str();

    Display::setCursor(0, 1);
    pad_string(msg);
    Display::print(msg_c_str);
    Serial.println(msg_c_str);

    proxi_friends_t friends = get_self_friends();
    Friends.init_friends(friends);

    std::string s = std::to_string(Friends.friends.size());

    msg = std::string("found ") + s + " friends";
    Serial.println(msg.c_str());

    init_mqtt_client();
    Serial.println("board initialized");

    Buzzer.playMelody(Chimes::startupChime, std::size(Chimes::startupChime));
    Buzzer.tillEnd();
    delay(1000);
    Buzzer.playTone({.freq = 440, .duration = 1000});
}

void setup()
{
    init_proxi();

// TODO: REMOVE MEE
#if IS_ARDUINO
    Messages.add_message("1", "b03fd37e-a6a8-0000-0000-000000000000", "hi hello");
#elif IS_ESP
    Messages.add_message("1", "1c180132-3337-3158-3333-74d12630574b", "hi hello");
#endif
    home_menu();
}

void status() { Serial.println("alive"); }
DebouncedCallback db_status(status, 1000);

void loop()
{
    state_check();
    Buzzer.update();

    if (mqtt != nullptr) {
        mqtt->poll();
    }
    static const std::vector<menu_item_title_t> top_level_items = {"friends", "messages",
                                                                   "send message", "send tone"};

    static const std::vector<std::string> preset_messages = {"hi", "hello", "yoo", "where u at?",
                                                             "call me"};

    static std::vector<std::string> dynamic_string_buffer;
    static std::vector<const char*> submenu_items;

    // Track recipient ID and friend keys across subviews
    static std::vector<std::string> friend_ids_buffer;
    static std::vector<std::string> tone_names_buffer;
    static std::string selected_friend_id;

    static enum class SUBVIEW {
        NONE,
        FRIENDS,
        MESSAGES,
        VIEW_MESSAGE,
        SEND_SELECT_FRIEND,
        SEND_SELECT_PRESET,
        SEND_TONE_SELECT_FRIEND,
        SEND_TONE_SELECT_TONE
    } active_subview = SUBVIEW::NONE;

    static size_t selected_message_idx = 0;

    bool btn_left = menu_btn_left.isPressed();
    bool btn_right = menu_btn_right.isPressed();
    bool btn_up = menu_btn_up.isPressed();
    bool btn_down = menu_btn_down.isPressed();

    // --- GLOBAL BACK (LEFT BUTTON) ---
    if (btn_left) {
        Buzzer.playTone({440, 50});

        if (active_subview == SUBVIEW::VIEW_MESSAGE) {
            // Opened messages are erased from the inbox, so rebuild the list
            build_message_previews(dynamic_string_buffer, submenu_items);

            if (submenu_items.empty()) {
                active_subview = SUBVIEW::NONE;
                Menu.set_items(top_level_items);
            } else {
                active_subview = SUBVIEW::MESSAGES;
                Menu.set_items(submenu_items);
                Menu.curr_idx = (selected_message_idx < submenu_items.size())
                                    ? selected_message_idx
                                    : submenu_items.size() - 1;
            }
        } else if (active_subview == SUBVIEW::SEND_SELECT_PRESET ||
                   active_subview == SUBVIEW::SEND_TONE_SELECT_TONE) {
            // Go back to friend selection (message or tone flow)
            active_subview = (active_subview == SUBVIEW::SEND_SELECT_PRESET)
                                 ? SUBVIEW::SEND_SELECT_FRIEND
                                 : SUBVIEW::SEND_TONE_SELECT_FRIEND;

            dynamic_string_buffer.clear();
            submenu_items.clear();

            for (const auto& id : friend_ids_buffer) {
                std::string friend_name;
                Friends.get_name(id, friend_name);
                dynamic_string_buffer.push_back(friend_name);
            }
            for (const auto& str : dynamic_string_buffer) {
                submenu_items.push_back(str.c_str());
            }

            Menu.set_items(submenu_items);
        } else if (active_subview != SUBVIEW::NONE) {
            active_subview = SUBVIEW::NONE;
            Menu.set_items(top_level_items);
        } else if (Menu.state() != MENU_STATE_ACTION::MENU_CLOSED) {
            Menu.close();
            home_menu();
        }
        delay(150);
        return;
    }

    // --- OPEN TOP MENU FROM HOME ---
    if (btn_right && (Menu.state() == MENU_STATE_ACTION::MENU_CLOSED)) {
        active_subview = SUBVIEW::NONE;
        Menu.set_items(top_level_items);
        Buzzer.playTone({440, 50});
        return;
    }

    // --- ACTIVE MENU NAVIGATION ---
    if (Menu.state() == MENU_STATE_ACTION::MENU_NAVIGATING) {
        // Cycle full messages directly using UP/DOWN
        if (active_subview == SUBVIEW::VIEW_MESSAGE) {
            size_t total_messages = Messages.messages.size();

            if (total_messages == 0) return;

            if (btn_up) {
                selected_message_idx =
                    (selected_message_idx == 0) ? (total_messages - 1) : (selected_message_idx - 1);
                Buzzer.playTone({440, 50});
                render_full_message(selected_message_idx);
                delay(150);
            } else if (btn_down) {
                selected_message_idx = (selected_message_idx + 1) % total_messages;
                Buzzer.playTone({440, 50});
                render_full_message(selected_message_idx);
                delay(150);
            }
            return;
        }

        if (btn_up) {
            Menu.prevItem();
            Buzzer.playTone({440, 50});
        } else if (btn_down) {
            Menu.nextItem();
            Buzzer.playTone({440, 50});
        } else if (btn_right) {
            Buzzer.playTone({440, 50});

            // 1. Viewing Message Previews -> Open Selected Message
            if (active_subview == SUBVIEW::MESSAGES) {
                if (Messages.messages.empty()) {
                    Display::clear();
                    Display::home();
                    Display::print("No messages");
                } else {
                    selected_message_idx = Menu.curr_idx;
                    active_subview = SUBVIEW::VIEW_MESSAGE;
                    render_full_message(selected_message_idx);

                    auto it = Messages.messages.begin();
                    std::advance(it, selected_message_idx);
                    Messages.read_message(it->first);
                }
            }
            // 2. Select Recipient Friend -> Open Preset Messages Menu
            else if (active_subview == SUBVIEW::SEND_SELECT_FRIEND) {
                if (!friend_ids_buffer.empty()) {
                    selected_friend_id = friend_ids_buffer[Menu.curr_idx];
                    active_subview = SUBVIEW::SEND_SELECT_PRESET;

                    dynamic_string_buffer.clear();
                    submenu_items.clear();

                    for (const auto& msg : preset_messages) {
                        dynamic_string_buffer.push_back(msg);
                    }
                    for (const auto& str : dynamic_string_buffer) {
                        submenu_items.push_back(str.c_str());
                    }

                    Menu.set_items(submenu_items);
                }
            }
            // 3. Select Preset Message -> Send Message & Show Confirmation
            else if (active_subview == SUBVIEW::SEND_SELECT_PRESET) {
                if (Menu.curr_idx < preset_messages.size()) {
                    std::string selected_msg = preset_messages[Menu.curr_idx];

                    Display::clear();
                    Display::setCursor(0, 0);
                    Display::print("Sending...");

                    bool sent = Messages.send_message(selected_friend_id, selected_msg);

                    Display::clear();
                    Display::setCursor(0, 0);
                    Display::print(sent ? "Message Sent!" : "Send Failed!");

                    Buzzer.playTone({static_cast<uint32_t>(sent ? 880 : 220), 300});
                    delay(1200);

                    // Reset state back to Top Menu
                    active_subview = SUBVIEW::NONE;
                    Menu.set_items(top_level_items);
                }
            }
            // 3b. Select Friend -> Open Tone List
            else if (active_subview == SUBVIEW::SEND_TONE_SELECT_FRIEND) {
                if (!friend_ids_buffer.empty()) {
                    selected_friend_id = friend_ids_buffer[Menu.curr_idx];
                    active_subview = SUBVIEW::SEND_TONE_SELECT_TONE;

                    dynamic_string_buffer.clear();
                    submenu_items.clear();
                    tone_names_buffer.clear();

                    for (const auto& kv : Chimes::tones) {
                        tone_names_buffer.push_back(kv.first);
                        dynamic_string_buffer.push_back(kv.first);
                    }
                    for (const auto& str : dynamic_string_buffer) {
                        submenu_items.push_back(str.c_str());
                    }

                    Menu.set_items(submenu_items);
                }
            }
            // 3c. Select Tone -> Send Tone & Show Confirmation
            else if (active_subview == SUBVIEW::SEND_TONE_SELECT_TONE) {
                if (Menu.curr_idx < tone_names_buffer.size()) {
                    Display::clear();
                    Display::setCursor(0, 0);
                    Display::print("Sending tone...");

                    bool sent =
                        Messages.send_tone(selected_friend_id, tone_names_buffer[Menu.curr_idx]);

                    Display::clear();
                    Display::setCursor(0, 0);
                    Display::print(sent ? "Tone Sent!" : "Send Failed!");

                    Buzzer.playTone({static_cast<uint32_t>(sent ? 880 : 220), 300});
                    delay(1200);

                    // Reset state back to Top Menu
                    active_subview = SUBVIEW::NONE;
                    Menu.set_items(top_level_items);
                }
            }
            // 4. Default Top Menu / Friends Selection
            else if (active_subview == SUBVIEW::NONE || active_subview == SUBVIEW::FRIENDS) {
                Menu.selectCurrItem();
            }
            delay(150);
        }
    }

    // --- SUBMENU DISPATCHER ---
    else if (Menu.state() == MENU_STATE_ACTION::MENU_SELECTED) {
        switch (Menu.selected()) {
            case 0: {  // Friends Selected
                active_subview = SUBVIEW::FRIENDS;
                dynamic_string_buffer.clear();
                submenu_items.clear();

                dynamic_string_buffer.reserve(Friends.friends.size());
                submenu_items.reserve(Friends.friends.size());

                for (const auto& kv : Friends.friends) {
                    dynamic_string_buffer.push_back(kv.second);
                }
                for (const auto& str : dynamic_string_buffer) {
                    submenu_items.push_back(str.c_str());
                }

                Menu.set_items(submenu_items);
                break;
            }
            case 1: {  // Messages Selected
                active_subview = SUBVIEW::MESSAGES;
                build_message_previews(dynamic_string_buffer, submenu_items);
                Menu.set_items(submenu_items);
                break;
            }
            case 2:    // Send Message Selected -> Step 1: Pick Friend
            case 3: {  // Send Tone Selected -> Step 1: Pick Friend
                const bool is_tone = (Menu.selected() == 3);
                active_subview =
                    is_tone ? SUBVIEW::SEND_TONE_SELECT_FRIEND : SUBVIEW::SEND_SELECT_FRIEND;

                dynamic_string_buffer.clear();
                submenu_items.clear();
                friend_ids_buffer.clear();

                if (Friends.friends.empty()) {
                    Display::clear();
                    Display::setCursor(0, 0);
                    Display::print("No friends found");
                    delay(1000);
                    Menu.set_items(top_level_items);
                    active_subview = SUBVIEW::NONE;
                    break;
                }

                for (const auto& kv : Friends.friends) {
                    friend_ids_buffer.push_back(kv.first);       // Store Friend ID (UUID)
                    dynamic_string_buffer.push_back(kv.second);  // Store Friend Name
                }

                for (const auto& str : dynamic_string_buffer) {
                    submenu_items.push_back(str.c_str());
                }

                Menu.set_items(submenu_items);
                break;
            }
        }
    }

    if (active_subview != SUBVIEW::VIEW_MESSAGE &&
        Menu.state() == MENU_STATE_ACTION::MENU_NAVIGATING) {
        Menu.display();
    }
}