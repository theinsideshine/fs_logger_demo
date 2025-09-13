// RtcNtp.cpp
#include "RtcNtp.h"

/**
 * @brief Clase para manejar la hora del sistema sincronizada vía NTP.
 * 
 * Guarda un `boot_epoch` con la última hora sincronizada y estima la hora actual
 * sumando el tiempo transcurrido desde entonces (`millis()`).
 * Debe inicializarse con `begin()` después de que el Wi-Fi esté conectado.
 */

/**
 * @brief Inicializa la configuración del cliente SNTP para sincronización horaria.
 *
 * Esta función debe llamarse una vez que el Wi-Fi esté completamente conectado.
 * Internamente ejecuta `configTime(...)` para configurar la zona horaria y los
 * servidores NTP, y realiza un primer intento de sincronización con `sync()`.
 *
 * ⚠️ IMPORTANTE:
 * - En modo Webserver, se debe llamar después de `WiFi.begin()` y conexión exitosa.
 * - En modo Blynk clásico (`Blynk.begin()`), la hora suele configurarse automáticamente,
 *   pero se recomienda llamar a `RtcNtp::begin()` igual por consistencia.
 * - En modo Blynk.Edgent, **es obligatorio** llamar a `RtcNtp::begin()` después de
 *   `BlynkEdgent.begin()`, ya que **Edgent NO configura NTP por su cuenta**.
 *
 * Esta función no espera a que la sincronización ocurra: para saber si la hora
 * fue obtenida exitosamente, usar `Rtc.sync()` y luego consultar `Rtc.isSynced()`.
 */

void RtcNtp::begin() {
    configTime(-3 * 3600, 0, "pool.ntp.org");  // Hora local Argentina (UTC-3)
    synced = sync();
}
/**

 * @brief Intenta sincronizar la hora desde los servidores NTP configurados.
 * Si getLocalTime(&tm) devuelve una hora válida, actualiza boot_epoch y
 * boot_millis para que now() pueda calcular el tiempo transcurrido.
 * Además, actualiza el flag synced, que puede consultarse con isSynced().
 * @return true si la hora fue sincronizada correctamente
 * @return false si no se pudo obtener la hora
 */
bool RtcNtp::sync() {
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
        boot_epoch = mktime(&timeinfo);
        boot_millis = millis();
        synced = true;
        return true;
    }
    synced = false;
    return false;
}

/**
* @brief Indica si el sistema obtuvo una hora válida desde NTP.
* Este flag se actualiza cada vez que se llama a sync().
* @return true si sync() obtuvo una hora válida, false si falló
*/

bool RtcNtp::isSynced() {
    return synced;
}

/**
* @brief Devuelve la hora actual como time_t, basada en boot_epoch + millis.
* Esta función debe usarse luego de una sincronización exitosa con sync().
* @return time_t en segundos desde Epoch (1970-01-01 UTC)
*/

time_t RtcNtp::now() {
    return boot_epoch + (millis() - boot_millis) / 1000;
}

/**
* @brief Devuelve la cantidad de segundos transcurridos desde medianoche local.
* Usa now() y lo convierte a tm para calcular los segundos desde 00:00:00.
* @return segundos desde medianoche (0–86399)
*/

uint32_t RtcNtp::secondsSinceMidnight() {
    time_t current = now();
    struct tm* tm_info = localtime(&current);
    return tm_info->tm_hour * 3600 + tm_info->tm_min * 60 + tm_info->tm_sec;
}
/**
* @brief Fuerza el valor base del reloj, como si se hubiera sincronizado.
* Esta función es útil para restaurar la hora desde una fuente externa
* o simulada, por ejemplo desde EEPROM o archivo.
* @param epoch valor de tiempo base (segundos desde Epoch)
*/
void RtcNtp::forceEpoch(time_t epoch) {
    boot_epoch = epoch;
    boot_millis = millis();
}
