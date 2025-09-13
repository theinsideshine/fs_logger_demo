#ifndef MSG_CAT_H
#define MSG_CAT_H

#include <Arduino.h>

// Catálogo de textos SOLO para Log.msg(...)
// Basado en los mensajes reales de tu sketch.
// Todo en PROGMEM con F(...) y firma compatible con Log.msg.

namespace Msg {

  // ===== Boot / App =====
  inline const __FlashStringHelper* APP_START()   { return F("boot: start build=%s"); }
  inline const __FlashStringHelper* LOOP_BANNER() { return F("fw=%s ip=%s"); }
  inline const __FlashStringHelper* HEARTBEAT()   { return F("heartbeat"); }

  // (Dejamos el resto ya listo, aunque HOY probamos sólo “boot”)
  inline const __FlashStringHelper* FS_MOUNT_OK()      { return F("fs: mounted (LittleFS)"); }
  inline const __FlashStringHelper* FS_MOUNT_FAIL()    { return F("fs: mount FAIL (LittleFS)"); }

  inline const __FlashStringHelper* WIFI_CONN_TRY()    { return F("wifi: connecting ssid=%s"); }
  inline const __FlashStringHelper* WIFI_CONN_OK()     { return F("wifi: connected ip=%s"); }
  inline const __FlashStringHelper* WIFI_CONN_TIMEOUT(){ return F("wifi: timeout retry"); }

  inline const __FlashStringHelper* NTP_SYNC_OK()      { return F("🛰️ RTC sincronizado vía NTP"); }
  inline const __FlashStringHelper* NTP_SYNC_FAIL()    { return F("❌ Falló sincronización NTP"); }
  inline const __FlashStringHelper* NTP_SYNC_WIFI_FAIL(){ return F("❌ Falló wifi en sincronización NTP"); }

  inline const __FlashStringHelper* BME_READING()      { return F("BME280 - T=%.2f°C H=%.2f%% VPD=%.2fkPa"); }
  inline const __FlashStringHelper* BME_INVALID()      { return F("BME280 - Lectura inválida"); }
  inline const __FlashStringHelper* TSL_READING()      { return F("TSL2561 - Light=%.1f%%  Min=%.1f  Max=%.1f  State=%d"); }
  inline const __FlashStringHelper* TSL_INVALID()      { return F("TSL2561 - Lectura inválida"); }

  inline const __FlashStringHelper* CFG_ENTER()        { return F("logWeb: ENTER (log cerrado para descarga)"); }
  inline const __FlashStringHelper* CFG_EXIT_NEW()     { return F("cfg: EXIT (new=%s)"); }
  inline const __FlashStringHelper* CFG_EXIT_FAIL()    { return F("cfg: EXIT (no se pudo abrir nuevo log)"); }

  inline const __FlashStringHelper* LOG_ROTATED()      { return F("log: rotated midnight"); }
  inline const __FlashStringHelper* LOG_FILE_CREATED() { return F("log: file created name=%s"); }
  inline const __FlashStringHelper* LOG_FILE_FAIL()    { return F("log: cannot create file serial-only"); }
}

#endif // MSG_CAT_H
