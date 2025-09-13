# ESP32 LittleFS Logger

> Este README documenta **el sistema de log que compartiste** (archivos `ClogFS.cpp/.h`, `LogWeb.*`, `RtcNtp.*`, `MsgCat.*` y el sketch `fs_logger_demo.ino`).  
> Es un *logger* de propósito general para ESP32 con **rotación diaria**, **protección por espacio libre** (*low‑water*), **buffer de arranque** y **interfaz web** para listar/descargar/borrar logs.

---

## ✨ Características

- **Log en LittleFS** con prefijo de tiempo `hh:mm:ss` (si hay hora válida).
- **Rotación diaria automática**: crea un archivo nuevo al cambiar el día.
- **Protección por “low‑water”**: mantiene una **reserva** de espacio libre.  
  Si `free < (bytes_de_la_próxima_línea + lowwater)` ⇒ **borra los logs** y abre uno nuevo.
- **Boot buffer**: si aún no hay archivo abierto, acumula en RAM y vuelca al abrir.
- **Web UI** (`/fs`): listar/ver/descargar/borrar y formatear el FS.
- **CLI por Serial**: modo web on/off, rotación de prueba, estadísticas de FS y pruebas de llenado.
- **Inyección de tiempo**: el logger no depende de una fuente de tiempo específica (usa un *provider* que vos definís).

---

## 🧩 Estructura (archivos principales)

```
/src
 ├─ ClogFS.h / ClogFS.cpp      // clase del logger a FS
 ├─ LogWeb.h / LogWeb.cpp      // web /fs (listar/ver/descargar/borrar/format)
 ├─ RtcNtp.h / RtcNtp.cpp      // NTP + proveedor de hora (opcional)
 ├─ MsgCat.h                   // textos tipo APP_START, HEARTBEAT, etc.
 └─ fs_logger_demo.ino         // ejemplo de uso (estados + CLI)
```



---

## ✅ Requisitos

- **Placa**: ESP32 / ESP32‑S3 (Arduino core 3.x recomendado).
- **Librerías**: `WiFi`, `LittleFS`, `FS`, `time.h` (incluidas en el core).

---

## ⚡ Configuración rápida (ejemplo mínimo)

```cpp
#include <LittleFS.h>
#include "ClogFS.h"
#include "RtcNtp.h"
#include "LogWeb.h"

RtcNtp  Rtc;
ClogFS  Log;
LogWeb  logWeb(80, "/");           // Web en /fs

time_t rtc_now_provider() {
  return Rtc.isSynced() ? Rtc.now() : 0;   // 0 => sin hora válida (no imprime hh:mm:ss)
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  // Montaje LittleFS (tal como está en tu código)
  LittleFS.begin(true, "/littlefs", 10, "littlefs");

  // 🧷 Config logger
  Log.setBootBufferCapacityBytes(4096);
  Log.setTimeProvider(rtc_now_provider);
  Log.setFsLowWater(2048);            // reserva (low-water) por defecto
  Log.setBasePath("/littlefs");       // clave: donde viven los logs

  // Iniciar web tras WiFi OK (ver demo para manejo de estados)
  // logWeb.begin();

  // Crear primer log
  time_t now = time(nullptr);
  String fname  = ClogFS::makeFilename(now);
  String header = "VERSION=demo MOTIVO_RESET=BOOT";
  Log.openFile(fname.c_str(), header.c_str());

  Log.msg(F("logger: ready name=%s"), fname.c_str());
}

void loop() {
  // Escribir periódicamente
  static uint32_t t0=0;
  if (millis()-t0 > 5000) {
    t0 = millis();
    Log.msg(F("heartbeat"));
  }

  // Rotación diaria
  Log.rotateDailyIfNeeded("MOTIVO_RESET=ROTATE");

  // Atender servidor web si lo usás
  // logWeb.loop();
}
```

> El *snippet* anterior es **representativo** de tu implementación actual. Si tu *loop* usa estados y CLI (como en `fs_logger_demo.ino`), mantenelo así: es más robusto.

---

## 📦 Montaje del FS y *basePath*

En el ejemplo y en tu código se monta así:

```cpp
LittleFS.begin(true, "/littlefs", 10, "littlefs");
```

Por lo tanto, los archivos de log viven bajo **`/littlefs`**.  
Configuralo en el logger para que el *wipe* borre lo que ves en la web:

```cpp
Log.setBasePath("/littlefs");
```

---

## 🔁 Rotación diaria

La rotación se dispara cuando cambia el día (campo `tm_mday`). Llamá en tu `loop()`:

```cpp
if (Log.rotateDailyIfNeeded("MOTIVO_RESET=ROTATE")) {
  // opcional: registrar evento de rotación
  Log.msg(F("log rotated"));
}
```

Los nombres de archivo siguen el patrón:

```
log-YYYYMMDD-HHMMSS.txt
```

---

## 🛡️ Protección por espacio (*low‑water*)

En cada escritura, el logger calcula:

- `need = strlen(line) + 1 + (tieneTS ? strlen(ts) : 0)`  
- `freeB = LittleFS.totalBytes() - LittleFS.usedBytes()`

Si se cumple:
```
freeB > 0 && freeB < (need + _fsLowWater)
```
entonces:
1) `closeFile()`  
2) `wipeAllInBasePath()` (borra archivos en `basePath`)  
3) `reopenFreshFileAfterWipe("FS_WIPE=1")` (crea uno nuevo con ese encabezado)  
4) Escribe la línea actual en el nuevo archivo.

Configuración típica:
```cpp
Log.setFsLowWater(2048);    // 2 KB de reserva
```

> Para **probar** la rama de *wipe* sin llenar el FS, podés subir temporalmente el *low‑water* por Serial (ver CLI) y escribir una línea.

---

## 🌐 Interfaz Web `/fs`

Incluida en `LogWeb` (tu implementación actual). Permite:

- **Listar** archivos de log
- **Ver** (render texto)
- **Descargar**
- **Borrar todos**
- **Formatear LittleFS**

**Modo Web (configuración):**  
Desde Serial:
```
cfg on   // cierra el archivo y pausa escrituras; habilita la web
cfg off  // abre un archivo nuevo y reanuda las escrituras
```

> Cuando la web está activa y el sistema en modo *cfg*, no se registran nuevas líneas para evitar truncamientos mientras descargás.

---

## ⌨️ CLI por Serial (incluido en el demo)

```
cfg on                      // entra a modo web (pausa logging)
cfg off                     // sale de modo web (reanuda y abre nuevo archivo)

fs                          // lista archivos (vía Log.listDir("/"))
fs stats                    // muestra total/used/free del FS
format                      // formatea (sólo en modo web)

rot try                     // intenta rotación diaria con la hora actual
rot +1d                     // simula +1 día y prueba rotación
rot reset                   // limpia el offset de simulación

log lowwater <bytes>        // fija la reserva de espacio (low-water)
log burst N [size]          // escribe N líneas (size máx 512) para pruebas
```

**Prueba mínima del wipe (didáctica):**
```
cfg off
fs stats                  // ver free
log lowwater 5000000      // poner umbral > free
log burst 1 16            // escribe 1 línea → wipe + FS_WIPE=1
log lowwater 2048         // volver a un valor normal
```

---

## 🧪 Notas de uso

- Si subís *low‑water* por encima del `free` actual, **cada** escritura provocará un *wipe*; usalo sólo para pruebas.
- `wipeAllInBasePath()` borra **archivos** (no directorios) en `basePath`.
- Si no tenés hora válida (`now() == 0`), se escribe **sin timestamp**, pero siempre a Serial.
- El *boot buffer* guarda todo lo registrado **antes** de abrir el primer archivo y lo vuelca al abrirlo.

---

## 🛠️ API pública de `ClogFS` (resumen)

```cpp
ClogFS();

// Config
void   setTimeProvider(time_t (*nowFn)());
void   setBootBufferCapacityBytes(size_t cap);
void   setFsLowWater(size_t bytes);     // reserva para “casi lleno”
void   setBasePath(const char* p);      // carpeta base para borrar/listar

// Archivo
bool   openFile(const char* filename, const char* header_ascii = nullptr);
bool   rotate(const String& newFilename, const char* header_ascii = nullptr);
bool   rotateDailyIfNeeded(const char* header_ascii = nullptr);
void   closeFile();
void   resetDayTracking();

// Log
void   msg(const __FlashStringHelper *fmt, ...);
void   ctrl(uint16_t raw, uint16_t filtered, uint8_t state, uint16_t danger_point);
void   log_sensor_with_state(const char* name, float value, float min, float max,
                             int state, float state_scale, const char* state_label);

// Utilidad
void   listDir(const char* path = "/");
static String makeFilename(time_t epoch);
```

---

## 🧭 Roadmap corto

- **Niveles de log** (`TRACE/DEBUG/INFO/WARN/ERROR`) con filtros para Serial/archivo.
- **Borrado selectivo** en lugar de *wipe* total (FIFO por fecha).
- **Métricas**: líneas/bytes escritos por archivo.

---

## 📄 Licencia

MIT. Podés usarlo, modificarlo y distribuirlo con libertad, manteniendo el aviso de licencia.
