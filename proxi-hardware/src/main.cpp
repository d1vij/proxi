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
#include <MFRC522.h>
#include <SPI.h>
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

// ---------------------------------------------------------------------------
// Dev settings
// ---------------------------------------------------------------------------

// How long (ms) to wait for the Serial Monitor to attach at boot.
// Needed on the UNO R4 WiFi because Serial is native USB (CDC): the port
// re-enumerates on every reset/upload and early prints are lost otherwise.
#ifndef SERIAL_WAIT_MS
#define SERIAL_WAIT_MS 5000
#endif

// NFC unlock timeout in ms. 0 = wait forever (production behaviour).
// While developing, a non-zero value lets the rest of the system start
// even if no card is presented / the reader isn't wired.
#ifndef NFC_UNLOCK_TIMEOUT_MS
#define NFC_UNLOCK_TIMEOUT_MS 15000
#endif

// Global Proxi Messages Controller
ProxiMessages Messages;

inline void print_board_info()
{
#if IS_ARDUINO
    Serial.println("[DEBUG] Board: Arduino UNO R4 WiFi");
    Serial.print("[DEBUG] Proxi ID: ");
    Serial.println(get_proxi_id().str.c_str());
#elif IS_ESP
    Serial.println("[DEBUG] Board: ESP-WROOM-32");
    Serial.print("[DEBUG] Proxi ID: ");
    Serial.println(get_proxi_id().str.c_str());
#else
#error "Unidentified Board Type!"
#endif
}

// Always returns something displayable, never skips a message
static std::string sender_display_name(const std::string& sender_id)
{
    std::string name;

    if (Friends.get_name(sender_id, name) && !name.empty()) return name;

    std::string lower = sender_id;
    for (auto& c : lower) c = static_cast<char>(tolower(c));
    if (Friends.get_name(lower, name) && !name.empty()) return name;

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
        if (friend_name.size() > 8) friend_name.resize(8);

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
    Serial.println("[DEBUG] Rendering Home Menu");
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

#if IS_ARDUINO
// Helper function to handle NFC authentication loop for Arduino
void authenticate_nfc()
{
    Serial.println("[DEBUG] [NFC] Starting SPI & PCD_Init...");
    SPI.begin();

    MFRC522 rfid(RC522_SS, RC522_RST);
    rfid.PCD_Init();
    delay(50);  // Short settling delay after reset

    Serial.print("[DEBUG] [NFC] Checking hardware firmware version... ");
    byte version = rfid.PCD_ReadRegister(MFRC522::VersionReg);
    Serial.print("0x");
    Serial.println(version, HEX);

    if (version == 0x00 || version == 0xFF) {
        Serial.println(
            "[ERROR] [NFC] MFRC522 Reader NOT detected! Check Wiring (SS, RST, SPI Pins).");
        Display::clear();
        Display::setCursor(0, 0);
        Display::print("NFC Error!");
        Display::setCursor(0, 1);
        Display::print("Check Reader");
        delay(3000);
        return;  // Exit to prevent soft-locking startup
    }

    MFRC522::MIFARE_Key key;
    for (byte i = 0; i < 6; i++) {
        key.keyByte[i] = 0xFF;  // Default factory MIFARE key
    }

    Display::clear();
    Display::setCursor(0, 0);
    Display::print("Scan NFC to");
    Display::setCursor(0, 1);
    Display::print("Unlock...");

    Serial.println("[DEBUG] [NFC] System Locked. Waiting for key card scan...");

    bool authenticated = false;
    uint32_t last_log = 0;
    const uint32_t lock_start = millis();

    while (!authenticated) {
        // Dev timeout so a missing card never soft-locks the whole startup
#if NFC_UNLOCK_TIMEOUT_MS > 0
        if (millis() - lock_start > NFC_UNLOCK_TIMEOUT_MS) {
            Serial.println("[WARN] [NFC] Unlock timeout - continuing startup WITHOUT auth.");
            Display::clear();
            Display::setCursor(0, 0);
            Display::print("NFC timeout");
            Display::setCursor(0, 1);
            Display::print("Skipping lock");
            delay(1000);
            return;
        }
#endif

        if (millis() - last_log > 3000) {
            Serial.println("[DEBUG] [NFC] Still scanning for cards...");
            last_log = millis();
        }

        // Look for new cards
        if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
            delay(100);
            continue;
        }

        Serial.println("[DEBUG] [NFC] Card detected!");

        // Authenticate with Block 4 (Sector 1)
        byte blockAddr = 4;
        MFRC522::StatusCode status =
            rfid.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockAddr, &key, &(rfid.uid));

        if (status == MFRC522::STATUS_OK) {
            byte buffer[18];
            byte size = sizeof(buffer);

            status = rfid.MIFARE_Read(blockAddr, buffer, &size);
            if (status == MFRC522::STATUS_OK) {
                std::string scanned_data = "";
                for (byte i = 0; i < 16; i++) {
                    if (buffer[i] != ' ' && buffer[i] != 0) {
                        scanned_data += (char)buffer[i];
                    }
                }

                Serial.print("[DEBUG] [NFC] Block 4 Read Content: ");
                Serial.println(scanned_data.c_str());

                // Check password match ("apple")
                if (scanned_data.find("apple") != std::string::npos) {
                    authenticated = true;

                    Display::clear();
                    Display::setCursor(0, 0);
                    Display::print("Access Granted!");
                    Serial.println("[DEBUG] [NFC] Authentication Successful!");

                    Buzzer.playTone({880, 200});
                    delay(1000);
                } else {
                    Display::clear();
                    Display::setCursor(0, 0);
                    Display::print("Wrong Card!");
                    Serial.println("[DEBUG] [NFC] Invalid card scanned!");

                    Buzzer.playTone({220, 400});
                    delay(1500);

                    Display::clear();
                    Display::setCursor(0, 0);
                    Display::print("Scan NFC to");
                    Display::setCursor(0, 1);
                    Display::print("Unlock...");
                }
            } else {
                Serial.print("[ERROR] [NFC] MIFARE Read failed: ");
                Serial.println(rfid.GetStatusCodeName(status));
            }
        } else {
            Serial.print("[ERROR] [NFC] Auth failed: ");
            Serial.println(rfid.GetStatusCodeName(status));
        }

        rfid.PICC_HaltA();
        rfid.PCD_StopCrypto1();
    }
}
#endif

void init_proxi()
{
    Serial.begin(115200);

    // UNO R4 WiFi uses native USB CDC: wait (bounded) for the host to open the
    // port, otherwise everything printed during boot is silently dropped.
    const uint32_t serial_t0 = millis();
    while (!Serial && (millis() - serial_t0) < SERIAL_WAIT_MS) {
        delay(10);
    }
    delay(200);  // Let the host finish attaching

    Serial.println("\n===========================");
    Serial.println("--- [PROXI STARTUP] ---");
    Serial.println("===========================");

    print_board_info();

    Serial.println("[DEBUG] Initializing Display...");
    Display::init();
    Display::clear();
    Display::home();
    Display::print("Display OK");
    Serial.println("[DEBUG] Display Init completed.");
    delay(500);

#if IS_ARDUINO
    // NFC lock screen before system startup (times out in dev, see NFC_UNLOCK_TIMEOUT_MS)
    authenticate_nfc();
#endif

    Display::clear();
    Display::home();
    Display::print("Setting up Proxi");

    Serial.println("[DEBUG] Initializing WiFi...");
    init_wifi();

    std::string msg = "fetching friends";
    Display::setCursor(0, 1);
    pad_string(msg);
    Display::print(msg.c_str());
    Serial.println(msg.c_str());

    proxi_friends_t friends = get_self_friends();
    Friends.init_friends(friends);

    std::string s = std::to_string(Friends.friends.size());
    msg = std::string("found ") + s + " friends";
    Serial.println(msg.c_str());

    Serial.println("[DEBUG] Initializing MQTT Client...");
    init_mqtt_client();
    Serial.println("[DEBUG] Board initialized successfully.");

    Buzzer.playMelody(Chimes::startupChime,
                      sizeof(Chimes::startupChime) / sizeof(Chimes::startupChime[0]));
    Buzzer.tillEnd();
    delay(1000);
    Buzzer.playTone({.freq = 440, .duration = 1000});
}

void setup()
{
    Serial.begin(115200);
    Serial.println("asd");
    init_proxi();

#if IS_ARDUINO
    Messages.add_message("1", "b03fd37e-a6a8-0000-0000-000000000000", "hi hello");
#elif IS_ESP
    Messages.add_message("1", "1c180132-3337-3158-3333-74d12630574b", "hi hello");
#endif
    home_menu();
}

// Simple millis-based heartbeat (the old DebouncedCallback was never invoked)
static void heartbeat()
{
    static uint32_t last = 0;
    if (millis() - last >= 5000) {
        last = millis();
        Serial.println("alive");
    }
}

void loop()
{
    return;
    heartbeat();
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
            } else if (active_subview == SUBVIEW::SEND_SELECT_FRIEND) {
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
            } else if (active_subview == SUBVIEW::SEND_SELECT_PRESET) {
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

                    active_subview = SUBVIEW::NONE;
                    Menu.set_items(top_level_items);
                }
            } else if (active_subview == SUBVIEW::SEND_TONE_SELECT_FRIEND) {
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
            } else if (active_subview == SUBVIEW::SEND_TONE_SELECT_TONE) {
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

                    active_subview = SUBVIEW::NONE;
                    Menu.set_items(top_level_items);
                }
            } else if (active_subview == SUBVIEW::NONE || active_subview == SUBVIEW::FRIENDS) {
                Menu.selectCurrItem();
            }
            delay(150);
        }
    }

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
        case 2:    // Send Message Selected
        case 3: {  // Send Tone Selected
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
                friend_ids_buffer.push_back(kv.first);
                dynamic_string_buffer.push_back(kv.second);
            }

            for (const auto& str : dynamic_string_buffer) {
                submenu_items.push_back(str.c_str());
            }

            Menu.set_items(submenu_items);
            break;
        }
    }

    if (active_subview != SUBVIEW::VIEW_MESSAGE &&
        Menu.state() == MENU_STATE_ACTION::MENU_NAVIGATING) {
        Menu.display();
    }
}