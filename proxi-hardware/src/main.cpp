#include <Arduino.h>
#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>

#include <iterator>
#include <string>
#include <vector>

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
#include "ProxiID.h"
#include "audio_samples.h"
#include "pins.h"
#include "platform.h"
#include "secrets.h"

WiFiClient wifi;
HttpClient http(wifi, "192.168.1.8", 3000);

void init_wifi()
{
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    Serial.print("Connecting");

    auto lcd = Display::instance();
    Display::setCursor(0, 1);
    Display::print("connecting wifi");

    while (WiFi.status() != WL_CONNECTED || WiFi.localIP() == IPAddress(0, 0, 0, 0)) {
        Serial.println("connecting");
        delay(200);
    }

    Display::clearSecondLine();
    Display::setCursor(0, 1);
    Display::print("wifi connected");

    delay(500);
    IPAddress ip = WiFi.localIP();

    std::string ip_str = ip.toString().c_str();
    Display::setCursor(0, 1);
    ip_str = "ip " + ip_str;

    Serial.println(ip_str.c_str());

    pad_string(ip_str);
    Display::print(ip_str.c_str());

    delay(500);
}

// TODO: ADD LOGGING FOR WIFI AND REQUEST EVENTS
void check_wifi_conn()
{
    // override whatever was being displayed
    Display::clear();
    Display::home();
    Display::print("wifi disconnected");
    Display::setCursor(0, 1);
    Display::print("connecting");

    while (WiFi.status() != WL_CONNECTED) {
        Serial.println("wifi disconnected");

        WiFi.disconnect();
        init_wifi();
    }
}
DebouncedCallback db_check_wifi_conn(check_wifi_conn, 5000);

// ---

inline void print_board_info()
{
#if IS_ARDUINO
    Serial.println("proxi running on Arduino UNO R4 WiFi");
    Serial.println(get_proxi_id().str);
#elif IS_ESP
    Serial.println("proxi running on ESP-WROOM-32");
    Serial.println(get_proxi_id().str);
#else
#error "which board are u using bro ??"
#endif
}

void setup()
{
    Serial.begin(115200);
    print_board_info();

    Display::init();
    Display::home();
    Display::print("setting up proxi");
    init_wifi();

    Serial.println("board initialized");

    Buzzer.playMelody(Chimes::startupChime, std::size(Chimes::startupChime));
    Buzzer.tillEnd();
    delay(1000);
    Buzzer.playTone({.freq = 440, .duration = 1000});
}

void status() { Serial.println("alive"); }
DebouncedCallback db_status(status, 1000);

void loop()
{
    // db_toggle_led.call();
    // db_check_wifi_conn.call();

    Buzzer.update();

    static std::vector<menu_item_title_t> items = {"first", "second", "third", "fourth"};

    if (menu_btn_left.isPressed() && (Menu.state() != MENU_STATE_ACTION::MENU_NAVIGATING)) {
        Menu.set_items(items);
    }

    if (Menu.state() == MENU_STATE_ACTION::MENU_NAVIGATING) {
        if (menu_btn_left.isPressed()) {
            Menu.
        }

        if (menu_btn_up.isPressed()) {
            Serial.println("up pressed");
            Menu.prevItem();
            Buzzer.playTone({440, 50});
        }

        else if (menu_btn_down.isPressed()) {
            Serial.println("down pressed");
            Menu.nextItem();
            Buzzer.playTone({440, 50});
        }
    }

    if (Menu.state() == MENU_STATE_ACTION::MENU_SELECTED) {
        if (Menu.has_updated()) {
            menu_item_title_t selected = items[Menu.selected()];
            // NOTE: I WAS HERE THINKING HOW THE MENU FLOW WOULD WORK OUT
            Display::
        }

        if (menu_btn_left.isPressed()) {
            Menu.close();
        }
    }

    Menu.display();
}
