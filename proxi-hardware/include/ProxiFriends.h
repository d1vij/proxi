#pragma once

#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>

#include <iterator>
#include <string>
#include <unordered_map>
#include <vector>

#include "ProxiID.h"
#include "connectivity.h"
#include "system_state.h"

// map of friend id vs friend name
typedef std::unordered_map<proxi_id_str_t, std::string> proxi_friends_t;

class ProxiFriends
{
   public:
    proxi_friends_t friends;

    ProxiFriends() {};

    void init_friends(proxi_friends_t initial_friends) { friends = std::move(initial_friends); }
    void add_friend(const proxi_id_t& friend_id, const std::string& name)
    {
        friends[friend_id.str] = name;
    }

    bool get_name(const proxi_id_str_t& friend_id, std::string& name) const
    {
        auto it = friends.find(friend_id);
        if (it != friends.end()) {
            name = it->second;
            return true;
        }
        return false;
    }
};

// singleton instance
static ProxiFriends Friends;

/**
 * fetches this proxi's friends from the hub
 */
static proxi_friends_t get_self_friends()
{
    std::string url = "/friends/" + get_proxi_id().str;
    Serial.println(url.c_str());

    if (http == nullptr) {
        unrecov_system_error("HTTP client not initialized");
    }

    http->get(url.c_str());
    Serial.println("got");

    int statusCode = http->responseStatusCode();
    Serial.print("HTTP Error Status: ");
    Serial.println(statusCode);

    if (statusCode != 200) {
        unrecov_system_error("unable to fetch friends");
    }

    Serial.println("p");
    JsonDocument doc;
    deserializeJson(doc, http->responseBody());

    // converting json root to a object
    JsonObject obj = doc.as<JsonObject>();

    // converting object to a unordered_map
    proxi_friends_t friends;

    for (JsonPair kv : obj) {
        std::string key = kv.key().c_str();
        std::string value = kv.value().as<String>().c_str();
        friends[key] = value;
    }

    return friends;
}