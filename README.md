# ESP32 LittleFS Logger — fs-logger-demo

> Sistema de log para ESP32 con **LittleFS**, **rotación diaria**, **reserva por low‑water**, **buffer de arranque**, **niveles de severidad** y **web /fs** para ver/descargar/borrar. Este README documenta los archivos `ClogFS.cpp/.h`, `LogWeb.*`, `RtcNtp.*`, `MsgCat.*` y el sketch `fs_logger_demo.ino`.

---

## Novedades clave
- **Severidad de logs** con prefijo textual: `TRACE`, `DEBUG`, `INFO`, `WARN`, `ERROR`, `CRIT`.
- **Filtro por umbral**: `Log.setMinSeverity(ClogFS::INFO)` muestra `INFO+` (WARN/ERROR/CRIT).  
  Cambiable en **runtime por Serial** con `log level <nivel>`.
- **Unificación de salida**: todos los mensajes pasan por la misma ruta (`Log.debug/info/warn/error/...`).  
  Se eliminaron helpers redundantes (p.ej. `log_sensor_with_state`) y **`ctrl`**.
- **`listDir()` ahora respeta severidad**: usa `DEBUG` para listado y `WARN` para errores.

Ejemplo de salida (ASCII):
```
VERSION=demo-6.1 MOTIVO_RESET=WDT? IP=192.168.0.142
INFO  10:22:12 boot: start build=demo-6.1
INFO  10:22:12 fs: mounted (LittleFS)
DEBUG 10:22:12 fs: found log-20250913-192654.txt (918B)
INFO  10:22:27 RTC sync via NTP
INFO  10:22:27 log: file created name=log-20250914-102227.txt
INFO  10:22:27 fw=demo-6.1 ip=192.168.0.142
DEBUG 10:22:35 hb.debug seq=1
INFO  10:22:35 hb.info seq=1
WARN  10:22:35 hb.warn seq=1
ERROR 10:22:35 hb.error seq=1
```

---

## ✨ Características
- **Log en LittleFS** con prefijo `hh:mm:ss` cuando hay hora válida (RTC/NTP).
- **Rotación diaria automática** al cambiar de día.
- **Protección por “low‑water”**: si `free < (bytes_a_escribir + lowwater)`, borra logs en la base y abre uno nuevo.
- **Boot buffer** (por defecto 4096 B) hasta que exista un archivo abierto.
- **Web `/fs`**: listar/ver/descargar/borrar y formatear el FS.
- **CLI por Serial** para pruebas (ver comandos abajo).
- **Inyección de tiempo** por callback (`setTimeProvider(...)`).

---

## 🧩 Estructura
```
/src
 ├─ ClogFS.h / ClogFS.cpp      // logger + severidad
 ├─ LogWeb.h / LogWeb.cpp      // web /fs (listar/ver/descargar/borrar/format)
 ├─ RtcNtp.h / RtcNtp.cpp      // NTP + proveedor de hora (opcional)
 ├─ MsgCat.h                   // textos de catálogo (ASCII)
 └─ fs_logger_demo.ino         // demo con estados y CLI por Serial
```

> Nota: se **eliminaron** `log_sensor_with_state` y `ctrl`. Si los ves referenciados, reemplazá por `Log.info(...)`, `Log.warn(...)`, etc.

---

## ✅ Requisitos
- **Placa**: ESP32/ESP32‑S3 (Arduino core 3.x recomendado).
- **Librerías**: `WiFi`, `LittleFS`, `FS`, `time.h` (incluidas en el core), y `WebServer` (incluida en el core ESP32).

---

## ⚡ Configuración mínima (snippet)
```cpp
#include <WiFi.h>
#include <LittleFS.h>
#include "ClogFS.h"
#include "RtcNtp.h"
#include "LogWeb.h"

RtcNtp  Rtc;
ClogFS  Log;
LogWeb  logWeb(80, "/");

time_t rtc_now_provider() {
  return Rtc.isSynced() ? Rtc.now() : 0;
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  LittleFS.begin(true, "/littlefs", 10, "littlefs");

  Log.setBootBufferCapacityBytes(4096);
  Log.setTimeProvider(rtc_now_provider);
  Log.setFsLowWater(2048);
  Log.setMinSeverity(ClogFS::INFO); // default: INFO+

  time_t now = time(nullptr);
  String fname  = ClogFS::makeFilename(now);
  String header = "VERSION=demo MOTIVO_RESET=BOOT IP=0.0.0.0";
  Log.openFile(fname.c_str(), header.c_str());

  Log.info(F("logger ready name=%s"), fname.c_str());
}

void loop() {
  // ejemplo de logs por severidad
  static uint32_t t0 = 0; static unsigned long seq = 0;
  if (millis() - t0 > 10000) {
    t0 = millis(); seq++;
    Log.debug(F("hb.debug seq=%lu"), seq);
    Log.info (F("hb.info seq=%lu"),  seq);
    Log.warn (F("hb.warn seq=%lu"),  seq);
    Log.error(F("hb.error seq=%lu"), seq);
  }

  // rotación diaria
  Log.rotateDailyIfNeeded("MOTIVO_RESET=ROTATE");
}
```

---

## 🎚️ Severidad y filtro
- Niveles: `TRACE`, `DEBUG`, `INFO`, `WARN`, `ERROR`, `CRIT`.
- **Umbral por código**:
  ```cpp
  Log.setMinSeverity(ClogFS::INFO); // muestra INFO/WARN/ERROR/CRIT
  ```
- **Cambiar en runtime por Serial**:
  ```
  log level TRACE   // muestra todo
  log level DEBUG   // DEBUG+
  log level INFO    // INFO+ (por defecto)
  log level WARN    // WARN/ERROR/CRIT
  log level ERROR   // ERROR/CRIT
  log level CRIT    // CRIT
  ```

---

## 🌐 Web `/fs` y modo configuración
Comandos por Serial:
```
cfg on     // pausa escrituras, cierra archivo, habilita web
cfg off    // reanuda, abre archivo nuevo y vuelca boot buffer
```
Mientras está en `cfg on`, lo que se loguea va a Serial y queda en el **boot buffer** para volcarse al reabrir.

---

## 🔁 Rotación y low‑water
- **Rotación diaria**: `Log.rotateDailyIfNeeded(header)`; los nombres son `log-YYYYMMDD-HHMMSS.txt`.
- **Low‑water**: si `free < (need + lowwater)`, hace `wipe` de la base y reabre.
  - Config: `Log.setFsLowWater(2048);`
  - Para forzar la rama de wipe en pruebas:
    ```
    log lowwater 5000000
    log burst 1 16
    log lowwater 2048
    ```

---

## ⌨️ CLI por Serial (demo)
```
cfg on / cfg off
fs                 // lista archivos (ahora por DEBUG/WARN)
fs stats           // total/used/free
format             // formatea (solo en cfg on)

rot try | rot +1d | rot reset

log lowwater <bytes>
log burst N [size]
log level <TRACE|DEBUG|INFO|WARN|ERROR|CRIT>
```

> `listDir()` ahora emite `DEBUG "fs: found ..."` y `WARN "fs: cannot open dir"`.

---

## Buenas prácticas
- Mantener **ASCII 7‑bit** en los textos que van al log.
- No incluir `.cpp` en el sketch; solo headers. `ClogFS.cpp` incluye `ClogFS.h`.
- Evitar *flush* excesivo; el logger ya gestiona volcados y *boot buffer*.
- Si no hay hora válida, el prefijo usa `millis()` en vez de `hh:mm:ss`.

---

## Licencia
MIT.
