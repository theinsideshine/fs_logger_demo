#include <Arduino.h>
#include <WiFi.h>
#include <LittleFS.h>
#include <time.h>

#include "RtcNtp.h"
#include "ClogFS.h"
#include "LogWeb.h"
#include "MsgCat.h"

// =========================
// Config
// =========================
static const char* WIFI_SSID   = "Pablo";
static const char* WIFI_PASS   = "01410398716";
static const char* FW_VERSION  = "demo-6.4";

// Instancias
RtcNtp  Rtc;
ClogFS  Log;
LogWeb  logWeb(80, "/");

// Flags
volatile bool g_logging_enabled = true;

// Estados
enum State { ST_INIT_FS=0, ST_WAIT_WIFI, ST_SYNC_NTP, ST_LOG_RUNNING };
static State st = ST_INIT_FS;

// Banner una vez
static bool g_loop_banner_done = false;

// Time provider
time_t rtc_now_provider() { return Rtc.isSynced() ? Rtc.now() : 0; }

// Simulacion de tiempo (opcional)
static long g_test_time_offset = 0;
time_t rtc_now_provider_testable() {
  time_t base = Rtc.isSynced() ? Rtc.now() : time(nullptr);
  return base + g_test_time_offset;
}

// Helpers
static String makePayload(size_t bytes) {
  const size_t MAX_PAYLOAD = 512;
  if (bytes > MAX_PAYLOAD) bytes = MAX_PAYLOAD;
  String s; s.reserve(bytes);
  for (size_t i = 0; i < bytes; ++i) s += 'X';
  return s;
}

static bool canWriteLogs() {
  if (logWeb.inWebMode()) {
    Serial.println(F("logWeb: en modo CFG (no se escribe). Usa 'cfg off' para reanudar."));
    return false;
  }
  if (!g_logging_enabled) {
    Serial.println(F("logging: pausado (g_logging_enabled = false)."));
    return false;
  }
  return true;
}

// Modo CFG
static void enter_cfg_mode() {
  Log.closeFile();
  g_logging_enabled = false;
  logWeb.enterWebMode();
  Log.info(Msg::CFG_ENTER());
}

static void exit_cfg_mode() {
  time_t now = Rtc.isSynced() ? Rtc.now() : time(nullptr);
  String fresh  = ClogFS::makeFilename(now);
  String header = String("VERSION=") + FW_VERSION +
                  " MOTIVO_RESET=CFG_EXIT IP=" + WiFi.localIP().toString();

  if (Log.openFile(fresh.c_str(), header.c_str())) {
    Log.info(Msg::CFG_EXIT_NEW(), fresh.c_str());
  } else {
    Log.error(Msg::CFG_EXIT_FAIL());
  }
  g_logging_enabled = true;
  logWeb.exitWebMode();
}

static void handleSerialCommands(){
  if(!Serial.available()) return;
  String line = Serial.readStringUntil('\n'); line.trim();

  if(line.equalsIgnoreCase("cfg on")){
    if(!logWeb.inWebMode()) enter_cfg_mode();

  } else if(line.equalsIgnoreCase("cfg off")){
    if(logWeb.inWebMode()) exit_cfg_mode();

  } else if(line.equalsIgnoreCase("fs")){
    Log.listDir("/");

  } else if(line.equalsIgnoreCase("format")){
    if (!logWeb.inWebMode()) {
      Serial.println(F("Entra a 'cfg on' primero"));
    } else {
      bool ok = LittleFS.format();
      Serial.printf("format: %s\n", ok ? "OK" : "FAIL");
    }

  } else if (line.equalsIgnoreCase("rot try")) {
    String header = String("VERSION=") + FW_VERSION +
                    " MOTIVO_RESET=TEST_ROTATE IP=" + WiFi.localIP().toString();
    bool did = Log.rotateDailyIfNeeded(header.c_str());
    Serial.printf("rot try -> %s\n", did ? "ROTATED" : "NO-ROTATE");

  } else if (line.equalsIgnoreCase("rot +1d")) {
    g_test_time_offset += 24 * 60 * 60;
    Serial.printf("time offset = %+ld sec\n", (long)g_test_time_offset);
    String header = String("VERSION=") + FW_VERSION +
                    " MOTIVO_RESET=TEST_ROTATE IP=" + WiFi.localIP().toString();
    bool did = Log.rotateDailyIfNeeded(header.c_str());
    Serial.printf("rot +1d -> %s\n", did ? "ROTATED" : "NO-ROTATE");

  } else if (line.equalsIgnoreCase("rot reset")) {
    g_test_time_offset = 0;
    Serial.println("time offset reset");

  } else if (line.startsWith("log lowwater")) {
    String arg = line.substring(String("log lowwater").length());
    arg.trim();
    int val = arg.toInt();
    if (val <= 0) {
      Serial.println(F("uso: log lowwater <bytes>  (ej: log lowwater 128)"));
    } else {
      Log.setFsLowWater((size_t)val);
      Serial.printf("nuevo fsLowWater = %u bytes\n", (unsigned)val);
    }

  } else if (line.startsWith("log burst")) {
    if (!canWriteLogs()) return;

    int N = 0, sz = 64;
    {
      int sp1 = line.indexOf(' ');
      int sp2 = (sp1 >= 0) ? line.indexOf(' ', sp1 + 1) : -1;
      int sp3 = (sp2 >= 0) ? line.indexOf(' ', sp2 + 1) : -1;
      if (sp2 > 0) N = line.substring(sp2 + 1, (sp3 > 0 ? sp3 : line.length())).toInt();
      if (sp3 > 0) sz = line.substring(sp3 + 1).toInt();
      if (N <= 0) N = 100;
      if (sz <= 0) sz = 64;
      if (sz > 512) sz = 512;
    }

    String payload = makePayload(sz);
    Serial.printf("log burst: N=%d size=%d\n", N, sz);
    for (int i = 1; i <= N; ++i) {
      Log.info(F("spam: i=%d/%d %s"), i, N, payload.c_str());
      delay(1);
    }

  } else if (line.equalsIgnoreCase("fs stats")) {
    size_t total = LittleFS.totalBytes();
    size_t used  = LittleFS.usedBytes();
    size_t freeB = (total >= used) ? (total - used) : 0;
    Serial.printf("FS total=%u used=%u free=%u\n",
                  (unsigned)total, (unsigned)used, (unsigned)freeB);

  } else if (line.startsWith("log level")) {
    String arg = line.substring(String("log level").length());
    arg.trim();
    ClogFS::Severity s = ClogFS::INFO;
    if      (arg.equalsIgnoreCase("TRACE")) s = ClogFS::TRACE;
    else if (arg.equalsIgnoreCase("DEBUG")) s = ClogFS::DEBUG;
    else if (arg.equalsIgnoreCase("INFO"))  s = ClogFS::INFO;
    else if (arg.equalsIgnoreCase("WARN"))  s = ClogFS::WARN;
    else if (arg.equalsIgnoreCase("ERROR")) s = ClogFS::ERROR;
    else if (arg.equalsIgnoreCase("CRIT"))  s = ClogFS::CRIT;
    else {
      Serial.println(F("niveles: TRACE|DEBUG|INFO|WARN|ERROR|CRIT"));
      return;
    }
    Log.setMinSeverity(s);
    Serial.printf("log level = %s\n", ClogFS::sevName(s));

  } else {
    Serial.println(F(
      "cmd: use 'cfg on', 'cfg off', 'fs', 'format',\n"
      "     'rot try', 'rot +1d', 'rot reset',\n"
      "     'log lowwater <bytes>', 'log burst N [size]',\n"
      "     'fs stats', 'log level <nivel>'\n"
    ));
  }
}

void setup(){
  Serial.begin(115200);
  delay(50);
  WiFi.mode(WIFI_STA);

  Log.setBootBufferCapacityBytes(4096);
  Log.setTimeProvider(rtc_now_provider);
  // Log.setTimeProvider(rtc_now_provider_testable);
  Log.setFsLowWater(2048);
  Log.setMinSeverity(ClogFS::INFO);  // default: INFO+

  Log.info(Msg::APP_START(), FW_VERSION);
  st = ST_INIT_FS;
}

void loop(){

  if (logWeb.inWebMode()) {
    logWeb.loop();
    handleSerialCommands();
    return;
  }

  handleSerialCommands();

  switch(st){
    case ST_INIT_FS: {
      if(!LittleFS.begin(true, "/littlefs", 10, "littlefs")){
        Log.error(Msg::FS_MOUNT_FAIL());
        delay(1000);
        break;
      }
      Log.info(Msg::FS_MOUNT_OK());
      Log.listDir("/");
      st = ST_WAIT_WIFI;
      break;
    }

    case ST_WAIT_WIFI: {
      if(WiFi.status()!=WL_CONNECTED){
        Log.info(Msg::WIFI_CONN_TRY(), WIFI_SSID);
        WiFi.begin(WIFI_SSID, WIFI_PASS);
        uint32_t t0 = millis();
        while(WiFi.status()!=WL_CONNECTED && (millis()-t0)<10000){ delay(250); }
        if(WiFi.status()==WL_CONNECTED){
          Log.info(Msg::WIFI_CONN_OK(), WiFi.localIP().toString().c_str());
          logWeb.begin();
          st = ST_SYNC_NTP;
        }else{
          Log.warn(Msg::WIFI_CONN_TIMEOUT());
          delay(1000);
        }
      }
      break;
    }

    case ST_SYNC_NTP: {
      Rtc.begin();
      if(!Rtc.isSynced()){
        Log.info(F("ntp: not synced yet retry"));
        uint32_t t0 = millis();
        while(!Rtc.sync() && (millis()-t0)<5000){ delay(500); }
      }
      if(Rtc.isSynced()){
        Log.info(Msg::NTP_SYNC_OK());
        time_t now = Rtc.now();
        String fname = ClogFS::makeFilename(now);
        String header = String("VERSION=") + FW_VERSION +
                        " MOTIVO_RESET=WDT? IP=" + WiFi.localIP().toString();
        if(Log.openFile(fname.c_str(), header.c_str())){
          Log.info(Msg::LOG_FILE_CREATED(), fname.c_str());
          Log.listDir("/");
          st = ST_LOG_RUNNING;
          g_loop_banner_done = false;
        }else{
          Log.error(Msg::LOG_FILE_FAIL());
          st = ST_LOG_RUNNING;
          g_loop_banner_done = false;
        }
      }else{
        Log.error(Msg::NTP_SYNC_FAIL());
        delay(1500);
      }
      break;
    }

    case ST_LOG_RUNNING: {
      if (!g_loop_banner_done) {
        Log.info(Msg::LOOP_BANNER(), FW_VERSION, WiFi.localIP().toString().c_str());
        g_loop_banner_done = true;
      }

      // === Heartbeats de prueba de severidad ===
      static uint32_t t_last = 0;
      static unsigned long hb_seq = 0;
      if (millis() - t_last > 10000 && g_logging_enabled) {
        t_last = millis();
        hb_seq++;

        Log.debug(F("hb.debug seq=%lu"), hb_seq);
        Log.info (F("hb.info seq=%lu"),  hb_seq);
        Log.warn (F("hb.warn seq=%lu"),  hb_seq);
        Log.error(F("hb.error seq=%lu"), hb_seq);
      }
      // ========================================

      String header = String("VERSION=") + FW_VERSION +
                      " MOTIVO_RESET=ROTATE IP=" + WiFi.localIP().toString();
      if (Log.rotateDailyIfNeeded(header.c_str())) {
        if (g_logging_enabled) {
          Log.info(Msg::LOG_ROTATED());
          Log.listDir("/");
        }
      }

      delay(10);
      break;
    }
  }
}

