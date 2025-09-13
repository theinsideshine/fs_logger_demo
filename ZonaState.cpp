// ZoneState.cpp
#include "ZoneState.h"

const char* zone_to_str(ZoneState z) {
    switch (z) {
        case ZONE_SAFE:       return "SEGURO";
        case ZONE_DANGER_MIN: return "PELIGRO_MIN";
        case ZONE_DANGER_MAX: return "PELIGRO_MAX";
        default:              return "DESCONOCIDO";
    }
}
