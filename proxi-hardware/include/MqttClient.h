#pragma once

#include <ArduinoJson.h>
#include <ArduinoMqttClient.h>

#include <string>

#include "AudioEngine.h"
#include "Display.h"
#include "ProxiID.h"
#include "ProxiMessages.h"
#include "audio_samples.h"
#include "connectivity.h"
#include "secrets.h"
#include "system_state.h"

static std::string SUB_MESSAGE;
static std::string SUB_TUNE;

inline MqttClient* mqtt = nullptr;

static void mqttMessageHandler(int messageSize)
{
    std::string topic = mqtt->messageTopic().c_str();

    if (topic == SUB_MESSAGE) {
        JsonDocument doc;
        deserializeJson(doc, *mqtt);

        std::string message_id = doc["message_id"].as<String>().c_str();
        std::string sender_id = doc["sender_id"].as<String>().c_str();
        std::string content = doc["content"].as<String>().c_str();

        Messages.add_message(message_id, sender_id, content);
        Buzzer.playTone({440, 50});
    } else if (topic == SUB_TUNE) {
        std::string tone_name;
        tone_name.reserve(messageSize);

        while (mqtt->available()) {
            tone_name += (char)mqtt->read();
        }

        auto it = Chimes::tones.find(tone_name);
        if (it != Chimes::tones.end()) {
            const std::vector<BuzzerNote>& tune = it->second;
            Buzzer.playMelody(tune.data(), tune.size());
        }
    }
}

static void init_mqtt_client()
{
    static WiFiClient mqtt_wifi;

    SUB_MESSAGE = "message/" + get_proxi_id().str;
    SUB_TUNE = "tone/" + get_proxi_id().str;

    Serial.println("[MQTT] connecting...");

    if (mqtt == nullptr) {
        mqtt = new MqttClient(mqtt_wifi);
    }

    mqtt->setId(get_proxi_id().str.c_str());
    mqtt->setKeepAliveInterval(30 * 1000);

    if (!mqtt->connect(HUB_IP, HUB_MQTT_PORT)) {
        unrecov_system_error(std::string("mqtt conn failed. ") +
                             std::to_string(mqtt->connectError()));
    }

    mqtt->onMessage(mqttMessageHandler);
    mqtt->subscribe(SUB_MESSAGE.c_str());
    mqtt->subscribe(SUB_TUNE.c_str());

    Serial.print("[MQTT] connected, subscribed to ");
    Serial.println(SUB_MESSAGE.c_str());
}