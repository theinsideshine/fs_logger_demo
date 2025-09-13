#ifndef RTC_NTP_H
#define RTC_NTP_H

#include <Arduino.h>
#include <time.h>


// --- Configuración de NTP ---
#define NTP_RETRY_INTERVAL_MS   1000   // Intervalo entre reintentos
#define NTP_TIMEOUT_MS          10000  // Tiempo máximo total para sincronizar
#define TIME_NTP_SYNC             86400000UL  // 24 horas en ms

class RtcNtp {
public:
    void begin();                         // Configura NTP y realiza un primer intento de sincronización
    bool sync();                          // Intenta sincronizar con el servidor NTP (actualiza boot_epoch)
    time_t now();                         // Devuelve el epoch actual estimado (boot_epoch + millis)
    uint32_t secondsSinceMidnight();      // Devuelve segundos desde medianoche (para evaluar horarios)
    void forceEpoch(time_t epoch);        // Setea manualmente la hora base (por ejemplo, desde EEPROM)
    bool isSynced();                      // Devuelve true si la última sincronización fue exitosa

private:
    time_t boot_epoch = 0;
    uint32_t boot_millis = 0;
    bool synced = false;
};

#endif // RTC_NTP_H
