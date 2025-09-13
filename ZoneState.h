// ZoneState.h
// Zona de estados de magnitud

#ifndef ZONE_STATE_H
#define ZONE_STATE_H

#include <Arduino.h>

#define ZONE

enum ZoneState {
    ZONE_DANGER_MIN = 0,
    ZONE_SAFE       = 1,
    ZONE_DANGER_MAX = 2
};

// ===== Claves de Events en Blynk =====
#define EVT_TEMP_ZONE   "temp_change"
#define EVT_HUM_ZONE    "hum_change"
#define EVT_VPD_ZONE    "vpd_change"
#define EVT_LUX_ZONE    "lux_change"


// Devuelve el texto en español para la zona
const char* zone_to_str(ZoneState z);

#endif // ZONE_STATE_H
