#pragma once

#include "platform.h"
#include "secrets.h"

#if IS_ARDUINO
#include <WiFiS3.h>
#else
#include <WiFi.h>
#endif
#include <ArduinoHttpClient.h>

#include "DebouncedCallback.h"
#include "Display.h"

static WiFiClient* wifi = nullptr;
static HttpClient* http = nullptr;

static void init_wifi()
{
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    Serial.print("Connecting");

    auto lcd = Display::instance();
    Display::setCursor(0, 1);
    Display::print("connecting wifi");

    while (WiFi.status() != WL_CONNECTED || WiFi.localIP() == IPAddress(0, 0, 0, 0)) {
        Serial.println("connecting");
        delay(500);
    }

    delete http;
    delete wifi;

    // init global singletons
    wifi = new WiFiClient();
    http = new HttpClient(*wifi, HUB_IP, HUB_PORT);

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

namespace
{

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

}  // namespace

static DebouncedCallback db_check_wifi_conn(check_wifi_conn, 5000);