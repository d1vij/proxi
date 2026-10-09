#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <inttypes.h>
#include <cstdio>
#include <string>

#include "platform.h"

constexpr size_t PROXI_ID_SIZE_BYTES = 16;
// 32 hex chars + 4 hyphens + 1 null terminator = 37 bytes
constexpr size_t PROXI_ID_STR_LEN = 36;

using proxi_id_str_t = std::string;

struct proxi_id_t {
    proxi_id_str_t str;
    uint8_t bytes[PROXI_ID_SIZE_BYTES];
};

inline proxi_id_t generate_proxi_id()
{
    proxi_id_t proxi_id = {};

#if IS_ARDUINO
    bsp_unique_id_t const* uid = R_BSP_UniqueIdGet();
    size_t copy_bytes = 16;
    memcpy(proxi_id.bytes, uid->unique_id_bytes, copy_bytes);
#else
    uint8_t mac[6] = {0};
    WiFi.macAddress(mac);
    memcpy(proxi_id.bytes, mac, sizeof(mac));
#endif

    // Standard UUID formatted string: 8-4-4-4-12
    char buffer[PROXI_ID_STR_LEN + 1];
    snprintf(buffer, sizeof(buffer),
             "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
             proxi_id.bytes[0], proxi_id.bytes[1], proxi_id.bytes[2], proxi_id.bytes[3],
             proxi_id.bytes[4], proxi_id.bytes[5], proxi_id.bytes[6], proxi_id.bytes[7],
             proxi_id.bytes[8], proxi_id.bytes[9], proxi_id.bytes[10], proxi_id.bytes[11],
             proxi_id.bytes[12], proxi_id.bytes[13], proxi_id.bytes[14], proxi_id.bytes[15]);

    proxi_id.str = buffer; // Assign cleanly to std::string

    return proxi_id;
}

/**
 * Lazy initialization via function static call
 */
inline const proxi_id_t& get_proxi_id()
{
    static const proxi_id_t id = generate_proxi_id();
    return id;
}