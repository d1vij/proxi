#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ArduinoMqttClient.h>

#include <string>
#include <unordered_map>
#include <vector>

#include "ProxiID.h"

// Defined in main.cpp
extern MqttClient* mqtt;

struct proxi_message_t {
    std::string message_id;
    proxi_id_str_t sender_id;
    std::string content;
};

class ProxiMessages
{
   public:
    std::unordered_map<std::string, proxi_message_t> messages;

    ProxiMessages() = default;

    void add_message(const std::string& message_id, const proxi_id_str_t& sender_id,
                     const std::string& content)
    {
        proxi_message_t message{message_id, sender_id, content};
        messages[message_id] = message;
    }

    void read_message(const std::string& message_id) { messages.erase(message_id); }

    bool send_message(proxi_id_str_t friend_id, std::string content)
    {
        if (mqtt == nullptr || !mqtt->connected()) {
            Serial.println("[MQTT] Cannot send - Client not connected!");
            return false;
        }

        std::string topic = "message/" + friend_id;

        JsonDocument doc;
        doc["message_id"] = std::to_string(millis()).c_str();
        doc["sender_id"] = get_proxi_id().str.c_str();
        doc["content"] = content.c_str();

        if (mqtt->beginMessage(topic.c_str()) == 0) {
            Serial.println("[MQTT] beginMessage failed");
            return false;
        }

        serializeJson(doc, *mqtt);

        if (mqtt->endMessage() == 1) {
            Serial.print("[MQTT] Sent successfully to ");
            Serial.println(topic.c_str());
            return true;
        }

        Serial.println("[MQTT] Transmission failed");
        return false;
    }

    bool send_tone(const proxi_id_str_t& friend_id, const std::string& tone_name)
    {
        if (mqtt == nullptr || !mqtt->connected()) {
            Serial.println("[MQTT] Cannot send tone - Client not connected!");
            return false;
        }

        std::string topic = "tone/" + friend_id;

        if (mqtt->beginMessage(topic.c_str()) == 0) {
            Serial.println("[MQTT] beginMessage failed");
            return false;
        }

        // Raw payload, no JSON: the receiver reads it byte by byte as the tone name
        mqtt->print(tone_name.c_str());

        if (mqtt->endMessage() == 1) {
            Serial.print("[MQTT] Tone sent to ");
            Serial.println(topic.c_str());
            return true;
        }

        Serial.println("[MQTT] Tone transmission failed");
        return false;
    }
};

// Declared after the class so the type is known
extern ProxiMessages Messages;