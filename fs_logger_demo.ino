// C:\Users\ptavolaro\AppData\Local\arduino\sketches
#include <Arduino.h>
#include <WiFi.h>
#include <LittleFS.h>
#include <time.h>

#include "RtcNtp.h"     // tu clase, sin cambios
#include "ClogFS.h"     // logger a FS con time provider (hh:mm:ss)
#include "LogWeb.h"     // página /fs para ver/descargar/borrar/formatear
#include "MsgCat.h"

// =========================
// Config
// =========================
static const char* WIFI_SSID   = "Pablo";
static const char* WIFI_PASS   = "01410398716";
static const char* FW_VERSION  = "demo-5.5";  // versión unificada

// Instancias
RtcNtp  Rtc;
ClogFS  Log;
LogWeb  logWeb(80, "/");

// Flag global para pausar escrituras en modo logWeb
volatile bool g_logging_enabled = true;

// Estados mínimos
enum State { ST_INIT_FS=0, ST_WAIT_WIFI, ST_SYNC_NTP, ST_LOG_RUNNING };
static State st = ST_INIT_FS;

// Banner una sola vez al entrar a ST_LOG_RUNNING
static bool g_loop_banner_done = false;

// === Proveedor de hora para el logger (inyección de dependencia)
time_t rtc_now_provider() {
  return Rtc.isSynced() ? Rtc.now() : 0;   // 0 => sin hora válida (no imprime hh:mm:ss)
}

// --- Simulador de tiempo para pruebas de rotación ---
static long g_test_time_offset = 0; // en segundos

time_t rtc_now_provider_testable() {
  time_t base = Rtc.isSynced() ? Rtc.now() : time(nullptr);
  return base + g_test_time_offset;
}

// =====================================================
// ============ HELPERS PARA PRUEBA DE LOG =============
// =====================================================
static String makePayload(size_t bytes) {
  // Limitar para no tensar el heap
  const size_t MAX_PAYLOAD = 512;
  if (bytes > MAX_PAYLOAD) bytes = MAX_PAYLOAD;
  String s; s.reserve(bytes);
  for (size_t i = 0; i < bytes; ++i) s += 'X';
  return s;
}

static bool canWriteLogs() {
  if (logWeb.inWebMode()) {
    Serial.println(F("logWeb: en modo CFG (no se escribe). Usá 'cfg off' para reanudar."));
    return false;
  }
  if (!g_logging_enabled) {
    Serial.println(F("logging: pausado (g_logging_enabled = false)."));
    return false;
  }
  return true;
}

// --------- Helpers modo logWeb (sellar log y pausar/reanudar) ---------
static void enter_cfg_mode() {
  // 1) cerrar el archivo actual (quede “sellado” y disponible para bajar)
  Log.closeFile();

  // 2) pausar nuevas escrituras
  g_logging_enabled = false;

  // 3) activar web de logWeb
  logWeb.enterWebMode();
  Log.msg(F("logWeb: ENTER (log cerrado para descarga)"));
}

static void exit_cfg_mode() {
  // 1) abrir un log nuevo y limpio
  time_t now = Rtc.isSynced() ? Rtc.now() : time(nullptr);
  String fresh  = ClogFS::makeFilename(now);
  String header = String("VERSION=") + FW_VERSION +
                  " MOTIVO_RESET=CFG_EXIT IP=" + WiFi.localIP().toString();

  if (Log.openFile(fresh.c_str(), header.c_str())) {
    Log.msg(F("cfg: EXIT (new=%s)"), fresh.c_str());
  } else {
    Log.msg(F("cfg: EXIT (no se pudo abrir nuevo log)"));
  }

  // 2) reanudar
  g_logging_enabled = true;

  // ⚠️ acá estaba el problema:
  // antes tenías logWeb.inWebMode(); (sólo consulta)
  // debería ser:
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
      Serial.println(F("Entrá a 'cfg on' primero"));
    } else {
      bool ok = LittleFS.format();
      Serial.printf("format: %s\n", ok ? "OK" : "FAIL");
    }

  // ------ comandos de prueba de rotación ------
  } else if (line.equalsIgnoreCase("rot try")) {
    String header = String("VERSION=") + FW_VERSION +
                    " MOTIVO_RESET=TEST_ROTATE IP=" + WiFi.localIP().toString();
    bool did = Log.rotateDailyIfNeeded(header.c_str());
    Serial.printf("rot try -> %s\n", did ? "ROTATED" : "NO-ROTATE");

  } else if (line.equalsIgnoreCase("rot +1d")) {
    g_test_time_offset += 24 * 60 * 60; // +1 día
    Serial.printf("time offset = %+ld sec\n", (long)g_test_time_offset);
    String header = String("VERSION=") + FW_VERSION +
                    " MOTIVO_RESET=TEST_ROTATE IP=" + WiFi.localIP().toString();
    bool did = Log.rotateDailyIfNeeded(header.c_str());
    Serial.printf("rot +1d -> %s\n", did ? "ROTATED" : "NO-ROTATE");

  } else if (line.equalsIgnoreCase("rot reset")) {
    g_test_time_offset = 0;
    Serial.println("time offset reset");

  // =====================================================
  // =========== PRIMERA PRUEBA: LOWWATER + BURST ========
  // =====================================================

  // Cambiar low-water en runtime (umbral de "lleno")
  } else if (line.startsWith("log lowwater")) {
    // Sintaxis: log lowwater <bytes>
    // Ej: log lowwater 128
    String arg = line.substring(String("log lowwater").length());
    arg.trim();
    int val = arg.toInt();
    if (val <= 0) {
      Serial.println(F("uso: log lowwater <bytes>  (ej: log lowwater 128)"));
    } else {
      Log.setFsLowWater((size_t)val);
      Serial.printf("nuevo fsLowWater = %u bytes\n", (unsigned)val);
    }

  // Generar N líneas con payload opcional para forzar el wipe+reopen rápido
  } else if (line.startsWith("log burst")) {
    if (!canWriteLogs()) return;

    // Sintaxis: log burst N [size]
    // N: cantidad de líneas; size: bytes de payload por línea (default 64, máx 512)
    int N = 0, sz = 64;

    // Parseo simple
    // "log burst 200 256"
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
      Log.msg(F("spam: i=%d/%d %s"), i, N, payload.c_str());
      delay(1); // ceder CPU/WiFi
    }

  } else if (line.equalsIgnoreCase("fs stats")) {
  size_t total = LittleFS.totalBytes();
  size_t used  = LittleFS.usedBytes();
  size_t freeB = (total >= used) ? (total - used) : 0;
  Serial.printf("FS total=%u used=%u free=%u\n",
                (unsigned)total, (unsigned)used, (unsigned)freeB);
  }
else {
    Serial.println(F(
      "cmd: use 'cfg on', 'cfg off', 'fs', 'format',\n"
      "     'rot try', 'rot +1d', 'rot reset',\n"
      "     'log lowwater <bytes>', 'log burst N [size]'\n, 'fs stasts'\n"
    ));
  }
}

void setup(){
  Serial.begin(115200);
  delay(50);
  WiFi.mode(WIFI_STA);

  Log.setBootBufferCapacityBytes(4096);
  Log.setTimeProvider(rtc_now_provider);
  //Log.setTimeProvider(rtc_now_provider_testable);
  Log.setFsLowWater(2048);  // margen de 2 KB antes de wipe (default); podés bajarlo por Serial

  Log.msg(Msg::APP_START(), FW_VERSION);
  st = ST_INIT_FS;
}

void loop(){

  // Si está en modo logWeb, sólo servir la página y frenar el resto
  if (logWeb.inWebMode()) {
    logWeb.loop();
    handleSerialCommands();
    return;
  }

  handleSerialCommands();

  switch(st){
    case ST_INIT_FS: {
      if(!LittleFS.begin(true, "/littlefs", 10, "littlefs")){
        Log.msg(F("fs: mount FAIL (LittleFS)"));
        delay(1000);
        break;
      }
      Log.msg(F("fs: mounted (LittleFS)"));
      Log.listDir("/"); 
      st = ST_WAIT_WIFI;
      break;
    }

    case ST_WAIT_WIFI: {
      if(WiFi.status()!=WL_CONNECTED){
        Log.msg(F("wifi: connecting ssid=%s"), WIFI_SSID);
        WiFi.begin(WIFI_SSID, WIFI_PASS);
        uint32_t t0 = millis();
        while(WiFi.status()!=WL_CONNECTED && (millis()-t0)<10000){ delay(250); }
        if(WiFi.status()==WL_CONNECTED){
          Log.msg(F("wifi: connected ip=%s"), WiFi.localIP().toString().c_str());

          // Iniciar WebServer SOLO luego de WiFi OK
          logWeb.begin();

          st = ST_SYNC_NTP;
        }else{
          Log.msg(F("wifi: timeout retry"));
          delay(1000);
        }
      }
      break;
    }

    case ST_SYNC_NTP: {
      Rtc.begin();
      if(!Rtc.isSynced()){
        Log.msg(F("ntp: not synced yet retry"));
        uint32_t t0 = millis();
        while(!Rtc.sync() && (millis()-t0)<5000){ delay(500); }
      }
      if(Rtc.isSynced()){
        time_t now = Rtc.now();
        String fname = ClogFS::makeFilename(now);
        String header = String("VERSION=") + FW_VERSION +
                        " MOTIVO_RESET=WDT? IP=" + WiFi.localIP().toString();
        if(Log.openFile(fname.c_str(), header.c_str())){
          Log.msg(F("log: file created name=%s"), fname.c_str());
          Log.listDir("/"); 
          st = ST_LOG_RUNNING;
          g_loop_banner_done = false;   // aseguramos que el banner salga al entrar
        }else{
          Log.msg(F("log: cannot create file serial-only"));
          st = ST_LOG_RUNNING;
          g_loop_banner_done = false;
        }
      }else{
        Log.msg(F("ntp: FAIL wait time valid"));
        delay(1500);
      }
      break;
    }

    case ST_LOG_RUNNING: {
      // ---------- Banner UNA sola vez al entrar al loop principal ----------
      if (!g_loop_banner_done) {
        Log.msg(Msg::LOOP_BANNER(), FW_VERSION, WiFi.localIP().toString().c_str());
        g_loop_banner_done = true;
      }
      // --------------------------------------------------------------------

      // Heartbeat opcional
      static uint32_t t_last = 0;
      if (millis() - t_last > 10000 && g_logging_enabled) {
        t_last = millis();
        Log.msg(Msg::HEARTBEAT());
      }

      // Rotación a medianoche (encapsulado)
      String header = String("VERSION=") + FW_VERSION +
                      " MOTIVO_RESET=ROTATE IP=" + WiFi.localIP().toString();
      if (Log.rotateDailyIfNeeded(header.c_str())) {
        if (g_logging_enabled) {
          Log.msg(Msg::LOG_ROTATED());
          Log.listDir("/");
        }
      }

      delay(10);
      break;
    }
  }
}
