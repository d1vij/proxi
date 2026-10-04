#pragma once

#include <Arduino.h>

const int PROXI_ID_SIZE_BYTES = 16;
const int PROXI_ID_STR_LEN = PROXI_ID_SIZE_BYTES * 2;  // two hex chars per byte

struct proxi_id_t {
    char str[PROXI_ID_STR_LEN + 1];
    byte bytes[PROXI_ID_SIZE_BYTES];
};

inline proxi_id_t generate_proxi_id()
{
    bsp_unique_id_t const* uid = R_BSP_UniqueIdGet();

    proxi_id_t proxi_id;
    snprintf(proxi_id.str, sizeof(proxi_id_t),
             "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
             uid->unique_id_bytes[0], uid->unique_id_bytes[1], uid->unique_id_bytes[2],
             uid->unique_id_bytes[3], uid->unique_id_bytes[4], uid->unique_id_bytes[5],
             uid->unique_id_bytes[6], uid->unique_id_bytes[7], uid->unique_id_bytes[8],
             uid->unique_id_bytes[9], uid->unique_id_bytes[10], uid->unique_id_bytes[11],
             uid->unique_id_bytes[12], uid->unique_id_bytes[13], uid->unique_id_bytes[14],
             uid->unique_id_bytes[15]);

    memcpy(proxi_id.bytes, uid->unique_id_bytes, PROXI_ID_SIZE_BYTES);
    return proxi_id;
}

/**
 * The Proxi ID for this Proxi Device
 */
inline const proxi_id_t PROXI_ID = generate_proxi_id();
