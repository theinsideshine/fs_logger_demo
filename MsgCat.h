#ifndef MSG_CAT_H
#define MSG_CAT_H

#include <Arduino.h>

namespace Msg {

  // -------------------------------------------------------------------
  // INFO
  // -------------------------------------------------------------------

  // Boot / Banner / Heartbeat
  inline const __FlashStringHelper* APP_START()        { return F("boot: start build=%s"); }
  inline const __FlashStringHelper* LOOP_BANNER()      { return F("fw=%s ip=%s"); }
  inline const __FlashStringHelper* HEARTBEAT()        { return F("heartbeat"); }

  // Proyecto / Firmware
  inline const __FlashStringHelper* PROJECT_NAME()     { return F("project: %s"); }
  inline const __FlashStringHelper* FW_VERSION()       { return F("firmware version: %s"); }

  // FS / LittleFS
  inline const __FlashStringHelper* FS_MOUNT_OK()      { return F("fs: mounted (LittleFS)"); }
  inline const __FlashStringHelper* FS_MOUNT_FAIL()    { return F("fs: mount FAIL (LittleFS)"); }
  inline const __FlashStringHelper* FS_LIST_FILE()     { return F("fs: found %s (%uB)"); }
  inline const __FlashStringHelper* FS_DIR_EMPTY()     { return F("fs: directory empty"); }
  inline const __FlashStringHelper* FS_WIPE_START()    { return F("fs: wipe start"); }
  inline const __FlashStringHelper* FS_WIPE_DONE()     { return F("fs: wipe done"); }
  inline const __FlashStringHelper* FS_STATS()         { return F("fs: total=%u used=%u free=%u"); }

  // Logs / Rotación
  inline const __FlashStringHelper* LOG_ROTATED()      { return F("log: rotated midnight"); }
  inline const __FlashStringHelper* LOG_FILE_CREATED() { return F("log: file created name=%s"); }
  inline const __FlashStringHelper* LOG_FILE_FAIL()    { return F("log: cannot create file serial-only"); }
  inline const __FlashStringHelper* LOG_LOW_WATER()    { return F("log: lowwater set=%uB"); }
  inline const __FlashStringHelper* LOG_BURST_START()  { return F("log: burst N=%d size=%d"); }
  inline const __FlashStringHelper* LOG_BURST_ITEM()   { return F("spam: i=%d/%d %s"); }

  // WiFi
  inline const __FlashStringHelper* WIFI_CONN_TRY()      { return F("wifi: connecting ssid=%s"); }
  inline const __FlashStringHelper* WIFI_CONN_OK()       { return F("wifi: connected ip=%s"); }
  inline const __FlashStringHelper* WIFI_CONN_TIMEOUT()  { return F("wifi: timeout retry"); }
  inline const __FlashStringHelper* WIFI_AP_MODE()       { return F("wifi: AP mode ssid=%s"); }

  // NTP / RTC
  inline const __FlashStringHelper* NTP_SYNC_TRY()       { return F("ntp: not synced yet retry"); }
  inline const __FlashStringHelper* NTP_SYNC_OK()        { return F("RTC sync via NTP"); }
  inline const __FlashStringHelper* NTP_SYNC_FAIL()      { return F("NTP sync FAIL"); }
  inline const __FlashStringHelper* NTP_SYNC_WIFI_FAIL() { return F("NTP sync FAIL (wifi)"); }

  // Web / CFG Mode
  inline const __FlashStringHelper* WEB_SERVER_START() { return F("web: server started"); }
  inline const __FlashStringHelper* WEB_SERVER_ON()    { return F("web: server enabled"); }
  inline const __FlashStringHelper* WEB_SERVER_OFF()   { return F("web: server disabled"); }

  inline const __FlashStringHelper* CFG_ENTER()        { return F("logWeb: ENTER (log closed for download)"); }
  inline const __FlashStringHelper* CFG_EXIT_NEW()     { return F("cfg: EXIT (new=%s)"); }
  inline const __FlashStringHelper* CFG_EXIT_FAIL()    { return F("cfg: EXIT (failed to open new log)"); }

  // Sensores / Estados
  inline const __FlashStringHelper* TIMER_SENSORS_START(){ return F("timer: sensors_sample started"); }
  inline const __FlashStringHelper* SENSOR_OK()        { return F("sensor: %s OK"); }
  inline const __FlashStringHelper* SENSOR_INIT()      { return F("sensor: %s init..."); }
  inline const __FlashStringHelper* SENSOR_VALUE()     { return F("sensor: %s=%s"); }
  inline const __FlashStringHelper* MODE_CFG()         { return F("mode: configuration"); }
  inline const __FlashStringHelper* MODE_RUNTIME()     { return F("mode: runtime"); }

  // Compat (sheet vieja)
  inline const __FlashStringHelper* BOOT_MSG()         { return F("Iniciando sistema..."); }
  inline const __FlashStringHelper* EDGENT_BEGIN()     { return F("BlynkEdgent.begin() ejecutado"); }
  inline const __FlashStringHelper* EDGENT_WIFI_WAIT() { return F("Esperando conexión Wi-Fi/Blynk (Edgent maneja los reintentos)"); }
  inline const __FlashStringHelper* NTP_WAITING()      { return F("Esperando sincronización NTP..."); }
  inline const __FlashStringHelper* CFG_EXIT_TO_APP()  { return F("Saliendo de CFG Blynk -> volver a APP"); }

  // -------------------------------------------------------------------
  // WARN
  // -------------------------------------------------------------------
  inline const __FlashStringHelper* NTP_NOT_SYNCED()   { return F("NTP aún no sincronizado..."); }
  inline const __FlashStringHelper* SENSOR_INVALID()   { return F("%s - Lectura inválida"); }
  inline const __FlashStringHelper* BME280_INVALID()   { return F("BME280 - Lectura inválida"); }
  inline const __FlashStringHelper* TSL2561_INVALID()  { return F("TSL2561 - Lectura inválida"); }
  inline const __FlashStringHelper* NTP_SYNC_FAIL_WARN(){ return F("Falló sincronización NTP"); }

  // -------------------------------------------------------------------
  // DEBUG
  // -------------------------------------------------------------------
  inline const __FlashStringHelper* INIT_IS_DAY_POST_NTP() {
    return F("init: is_day (post-NTP): %02d:%02d:%02d day_start=%s night_start=%s => is_day=%d");
  }
  inline const __FlashStringHelper* ISDAY_STATUS() {
    return F("ISDAY %02d:%02d:%02d day_start=%s night_start=%s prev=%d -> now=%d");
  }
  inline const __FlashStringHelper* ISDAY_CHANGE() {
    return F("Cambio is_day: %d -> %d (H=%02d:%02d:%02d | day_start=%s | night_start=%s)");
  }
  inline const __FlashStringHelper* INIT_ZONA_TEMP()   { return F("init zona temp: value=%.2f°C, estado=%d"); }
  inline const __FlashStringHelper* INIT_ZONA_HUM()    { return F("init zona hum: value=%.2f%%, estado=%d"); }
  inline const __FlashStringHelper* INIT_ZONA_VPD()    { return F("init zona vpd: value=%.2f kPa, estado=%d"); }
  inline const __FlashStringHelper* INIT_ZONA_LUX()    { return F("init zona lux: value=%.2f%%, estado=%d"); }
  inline const __FlashStringHelper* BME280_LINE()      { return F("BME280 - T=%.2f°C H=%.2f%% VPD=%.2f kPa"); }
  inline const __FlashStringHelper* V100_TEMP_SENT()   { return F("V100 -> %.2f°C temperatura enviada a Blynk."); }

  // -------------------------------------------------------------------
  // ERROR
  // -------------------------------------------------------------------
  inline const __FlashStringHelper* BME280_ERROR()     { return F("BME280 ERROR"); }
  inline const __FlashStringHelper* TSL2561_ERROR()    { return F("TSL2561 ERROR"); }
  inline const __FlashStringHelper* NTP_TIMEOUT()      { return F("Timeout: no se pudo sincronizar NTP"); }
  inline const __FlashStringHelper* INIT_ZONA_TEMP_FAIL() { return F("Init zona temp: lectura inválida"); }
  inline const __FlashStringHelper* INIT_ZONA_HUM_FAIL()  { return F("Init zona hum: lectura inválida"); }
  inline const __FlashStringHelper* INIT_ZONA_LUX_FAIL()  { return F("Init zona lux: lectura inválida"); }
  inline const __FlashStringHelper* NTP_SYNC_WIFI_FAIL_ERR(){ return F("Falló wifi en sincronización NTP"); }

} // namespace Msg

#endif // MSG_CAT_H



/*
INFO
Log.info(Msg::APP_START(), FW_VERSION);
Log.info(Msg::WIFI_CONN_TRY(), WIFI_SSID);
Log.info(Msg::WIFI_CONN_OK(), WiFi.localIP().toString().c_str());
Log.info(Msg::LOG_FILE_CREATED(), fname.c_str());
Log.info(Msg::FS_STATS(), (unsigned)LittleFS.totalBytes(), (unsigned)LittleFS.usedBytes(), (unsigned)(LittleFS.totalBytes()-LittleFS.usedBytes()));
Log.info(Msg::SENSOR_OK(), "BME280");

WARN
Log.warn(Msg::NTP_NOT_SYNCED());
Log.warn(Msg::BME280_INVALID());
Log.warn(Msg::TSL2561_INVALID());
Log.warn(Msg::NTP_SYNC_FAIL_WARN());

DEBUG
// 1) init post NTP
Log.debug(Msg::INIT_IS_DAY_POST_NTP(), tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec,
          dayStr.c_str(), nightStr.c_str(), (int)d);

// 2) estados de zonas
Log.debug(Msg::INIT_ZONA_TEMP(),  value_temp,  (int)zone_state_temp);
Log.debug(Msg::INIT_ZONA_HUM(),   value_hum,   (int)zone_state_hum);
Log.debug(Msg::INIT_ZONA_VPD(),   value_vpd,   (int)zone_state_vpd);
Log.debug(Msg::INIT_ZONA_LUX(),   value_lux,   (int)zone_state_light);

// 3) línea BME280
Log.debug(Msg::BME280_LINE(), tC, hPct, vpdKpa);

// 4) envío a Blynk (V100)
Log.debug(Msg::V100_TEMP_SENT(), tC);

// 5) status y cambios de is_day
Log.debug(Msg::ISDAY_STATUS(), tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec,
          dayStr.c_str(), nightStr.c_str(), (int)prev, (int)is_day_now);
Log.debug(Msg::ISDAY_CHANGE(), (int)prev, (int)is_day_now,
          tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec,
          dayStr.c_str(), nightStr.c_str());

*/


