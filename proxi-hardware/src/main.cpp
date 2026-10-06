#include <Arduino.h>
#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>

#include <iterator>
#include <string>

#if IS_ARDUINO
#include <WiFiS3.h>
#else
#include <WiFi.h>
#endif

#include <AudioEngine.h>

#include "DebouncedButton.h"
#include "DebouncedCallback.h"
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

    while (WiFi.status() != WL_CONNECTED || WiFi.localIP() == IPAddress(0, 0, 0, 0)) {
        delay(200);
        Serial.print('.');
    }

    IPAddress ip = WiFi.localIP();
    Serial.print("Local IP: ");
    Serial.println(ip.toString());
}

AudioEngine buzzer(BUZZER);

void setup()
{
    Serial.begin(115200);

#if IS_ARDUINO
    Serial.println("Hello from arduino");
#elif IS_ESP
    Serial.println("Hello from esp32");
#else
#error "which board are u using bro ??"
#endif

    init_wifi();

    pinMode(LED, OUTPUT);

    Serial.println("board initialized");

    http.get("/led/on");
    JsonDocument doc;
    deserializeJson(doc, http.responseBody());

    state = doc["led"];

    buzzer.playMelody(Chimes::startupChime, std::size(Chimes::startupChime));
}

void check_wifi_conn()
{
    while (WiFi.status() != WL_CONNECTED) {
        Serial.println("wifi disconnected");
        WiFi.disconnect();
        init_wifi();
    }
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
    buzzer.update();
    db_toggle_led.call();
    db_check_wifi_conn.call();
}
