#ifndef MSG_CAT_H
#define MSG_CAT_H

#include <Arduino.h>

namespace Msg {
  inline const __FlashStringHelper* APP_START()    { return F("boot: start build=%s"); }
  inline const __FlashStringHelper* LOOP_BANNER()  { return F("fw=%s ip=%s"); }
  inline const __FlashStringHelper* HEARTBEAT()    { return F("heartbeat"); }

  inline const __FlashStringHelper* FS_MOUNT_OK()      { return F("fs: mounted (LittleFS)"); }
  inline const __FlashStringHelper* FS_MOUNT_FAIL()    { return F("fs: mount FAIL (LittleFS)"); }

  inline const __FlashStringHelper* WIFI_CONN_TRY()     { return F("wifi: connecting ssid=%s"); }
  inline const __FlashStringHelper* WIFI_CONN_OK()      { return F("wifi: connected ip=%s"); }
  inline const __FlashStringHelper* WIFI_CONN_TIMEOUT() { return F("wifi: timeout retry"); }

  inline const __FlashStringHelper* NTP_SYNC_OK()        { return F("RTC sync via NTP"); }
  inline const __FlashStringHelper* NTP_SYNC_FAIL()      { return F("NTP sync FAIL"); }
  inline const __FlashStringHelper* NTP_SYNC_WIFI_FAIL() { return F("NTP sync FAIL (wifi)"); }

  inline const __FlashStringHelper* LOG_ROTATED()        { return F("log: rotated midnight"); }
  inline const __FlashStringHelper* LOG_FILE_CREATED()   { return F("log: file created name=%s"); }
  inline const __FlashStringHelper* LOG_FILE_FAIL()      { return F("log: cannot create file serial-only"); }

  inline const __FlashStringHelper* CFG_ENTER()          { return F("logWeb: ENTER (log closed for download)"); }
  inline const __FlashStringHelper* CFG_EXIT_NEW()       { return F("cfg: EXIT (new=%s)"); }
  inline const __FlashStringHelper* CFG_EXIT_FAIL()      { return F("cfg: EXIT (failed to open new log)"); }
}

#endif // MSG_CAT_H



