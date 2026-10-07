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
#include "ProxiID.h"
// #include "MenuState.h"
#include "audio_samples.h"
#include "pins.h"
#include "platform.h"
#include "secrets.h"

WiFiClient wifi;
HttpClient http(wifi, "192.168.1.8", 3000);

bool state = false;

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

    pinMode(LED, OUTPUT);

    Serial.println("board initialized");

    Buzzer.playMelody(Chimes::startupChime, std::size(Chimes::startupChime));
    Buzzer.tillEnd();
    delay(1000);
    Buzzer.playTone({.freq = 440, .duration = 1000});

    // std::vector<menu_item_title_t> items = {"first", "second"};
    // Menu.display(items);
}

void toggle_led()
{
    JsonDocument doc;
    doc["current"] = state;

    String body;
    serializeJson(doc, body);

    http.post("/led/toggle", "application/json", body);

    deserializeJson(doc, http.responseBody());
    state = doc["led"];

    digitalWrite(LED, state);
}

DebouncedCallback db_check_wifi_conn(check_wifi_conn, 5000);
DebouncedCallback db_toggle_led(toggle_led, 1000);

void loop()
{
    Buzzer.update();
    // db_toggle_led.call();
    // db_check_wifi_conn.call();
}
