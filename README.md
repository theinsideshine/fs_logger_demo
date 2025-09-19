# ESP32 LittleFS Logger — fs-logger-demo

> Sistema de log para ESP32 con **LittleFS**, **rotación diaria**, **reserva por low-water**, **buffer de arranque**, **niveles de severidad**, **modos de salida** y **web /fs** para ver/descargar/borrar.  
> Este README documenta los archivos `ClogFS.cpp/.h`, `LogWeb.*`, `RtcNtp.*`, `MsgCat.*` y el sketch `fs_logger_demo.ino`.

---

## Novedades clave
- **Severidad de logs** con prefijo textual: `TRACE`, `DEBUG`, `INFO`, `WARN`, `ERROR`, `CRIT`.
- **Filtro por umbral**: `Log.setMinSeverity(ClogFS::INFO)` muestra `INFO+` (WARN/ERROR/CRIT).  
  Cambiable en **runtime por Serial** con `log level <nivel>`.
- **Modos de salida (`out mode`)**: elegí si los logs van a `Serial`, `LittleFS`, ambos, o se desactivan.
- **Unificación de salida**: todos los mensajes pasan por la misma ruta (`Log.debug/info/warn/error/...`).
- **`listDir()` ahora respeta severidad**: usa `DEBUG` para listado y `WARN` para errores.

Ejemplo de salida:
```
VERSION=demo-6.4 MOTIVO_RESET=WDT? IP=192.168.0.142
INFO  10:22:12 boot: start build=demo-6.4
INFO  10:22:12 fs: mounted (LittleFS)
DEBUG 10:22:12 fs: found log-20250913-192654.txt (918B)
INFO  10:22:27 RTC sync via NTP
INFO  10:22:27 log: file created name=log-20250914-102227.txt
INFO  10:22:27 fw=demo-6.4 ip=192.168.0.142
DEBUG 10:22:35 hb.debug seq=1
INFO  10:22:35 hb.info seq=1
WARN  10:22:35 hb.warn seq=1
ERROR 10:22:35 hb.error seq=1
```

---

## ✨ Características
- **Log en LittleFS** con prefijo `hh:mm:ss` cuando hay hora válida (RTC/NTP).
- **Rotación diaria automática** al cambiar de día.
- **Protección por “low-water”**: si `free < (bytes_a_escribir + lowwater)`, borra logs en la base y abre uno nuevo.
- **Boot buffer** (por defecto 4096 B) hasta que exista un archivo abierto.
- **Web `/fs`**: listar/ver/descargar/borrar y formatear el FS.
- **CLI por Serial** para pruebas (ver comandos abajo).
- **Inyección de tiempo** por callback (`setTimeProvider(...)`).

---

## 🧩 Estructura
```
/src
 ├─ ClogFS.h / ClogFS.cpp      // logger + severidad + modos salida
 ├─ LogWeb.h / LogWeb.cpp      // web /fs (listar/ver/descargar/borrar/format)
 ├─ RtcNtp.h / RtcNtp.cpp      // NTP + proveedor de hora (opcional)
 ├─ MsgCat.h                   // catálogo de mensajes (INFO/WARN/DEBUG/ERROR)
 └─ fs_logger_demo.ino         // demo con estados y CLI por Serial
```

---

## ✅ Requisitos
- **Placa**: ESP32/ESP32-S3 (Arduino core 3.x recomendado).
- **Librerías**: `WiFi`, `LittleFS`, `FS`, `time.h` (core ESP32), y `WebServer` (incluida en el core).

---

## ⚡ Configuración mínima
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
  Log.setLevel(ClogFS::LVL_SERIAL_AND_LOG); // default: serial+log

  time_t now = time(nullptr);
  String fname  = ClogFS::makeFilename(now);
  String header = "VERSION=demo MOTIVO_RESET=BOOT IP=0.0.0.0";
  Log.openFile(fname.c_str(), header.c_str());

  Log.info(F("logger ready name=%s"), fname.c_str());
}
```

---

## 🎚️ Severidad
- Niveles: `TRACE`, `DEBUG`, `INFO`, `WARN`, `ERROR`, `CRIT`.
- **Umbral por código**:
  ```cpp
  Log.setMinSeverity(ClogFS::INFO); // muestra INFO/WARN/ERROR/CRIT
  ```
- **Cambiar en runtime (Serial)**:
  ```
  log level TRACE
  log level DEBUG
  log level INFO
  log level WARN
  log level ERROR
  log level CRIT
  ```

---

## 🖧 Modos de salida (`out mode`)
Define **dónde** van los logs:
- `off` → desactiva toda salida.
- `serial` → solo imprime en consola.
- `log` / `fs` → solo guarda en LittleFS.
- `serial+log` → imprime en consola **y** guarda en archivo.

**Ejemplos (Serial):**
```
out mode off
out mode serial
out mode log
out mode serial+log
```

**Por código:**
```cpp
Log.setLevel(ClogFS::LVL_OFF);
Log.setLevel(ClogFS::LVL_SERIAL);
Log.setLevel(ClogFS::LVL_LOG_ONLY);
Log.setLevel(ClogFS::LVL_SERIAL_AND_LOG);
```

---

## 🌐 Web `/fs` y modo configuración
```
cfg on     // pausa escrituras, cierra archivo, habilita web
cfg off    // reanuda, abre archivo nuevo y vuelca boot buffer
```

Cuando el ESP32 está en modo **cfg on**, se abre un servidor HTTP en el puerto 80.  
Accediendo a `http://<IP_DEL_ESP>/fs` en el navegador se muestra:

- **Listado de archivos de log** (`log-YYYYMMDD-HHMMSS.txt`).
- **Acciones**: ver, descargar o borrar cada archivo.
- **Botón [Format FS]** para limpiar el sistema de archivos.

Ejemplo de vista:
```
[FS LIST]
 log-20250918-102200.txt (4.2 KB)   [View] [Download] [Delete]
 log-20250919-091500.txt (3.8 KB)   [View] [Download] [Delete]

[FS Actions]
 [Format FS]   [Exit CFG mode]
```

⚠️ Mientras está en `cfg on`, no se escribe en FS (solo Serial + boot buffer).  
Al volver con `cfg off`, se crea un nuevo archivo y se vuelcan los logs pendientes.

---

## 🔁 Rotación y low-water
- **Rotación diaria**: `Log.rotateDailyIfNeeded(header)`.
- **Low-water**: `Log.setFsLowWater(2048);`

---

## ⌨️ CLI por Serial (demo)
```
cfg on / cfg off
fs
fs stats
format
rot try | rot +1d | rot reset
log lowwater <bytes>
log burst N [size]
log level <TRACE|DEBUG|INFO|WARN|ERROR|CRIT>
out mode <off|serial|log|serial+log>
```

---

## Buenas prácticas
- Usar solo **ASCII 7-bit** en textos.
- Evitar `.cpp` en el sketch; incluir solo headers.
- No usar flush manual; el logger ya gestiona boot buffer y volcados.
- Sin RTC/NTP válido, se antepone `millis()`.

---

## Licencia
MIT.

---

# Simulación de Rotación Diaria y Low‑Water · fs-logger-demo

Este documento explica **cómo probar** la rotación automática de logs por **cambio de día** y el mecanismo de **reserva low‑water** (limpieza del FS cuando queda poco espacio) sin esperar condiciones reales.

> Requisitos: firmware del demo (`fs_logger_demo.ino`) con comandos por Serial habilitados y `ClogFS` con `rotateDailyIfNeeded(...)` y `setFsLowWater(...)` integrados.

---

## 📆 1) Simular Rotación Diaria

La demo incluye un **offset de tiempo** solo para pruebas. Tenés tres comandos clave:

```
rot try       // intenta rotar si detecta cambio de día real
rot +1d       // avanza 1 día simulado y prueba rotación
rot reset     // vuelve el offset a 0 (tiempo normal)
```

### Pasos sugeridos

1. **Ver el archivo actual** (cuando el logger ya abrió un nombre basado en fecha):
   ```
   fs
   ```
   Deberías ver algo como:
   ```
   fs: found log-20250919-091500.txt (3.8 KB)
   ```

2. **Forzar avance de un día** y pedir rotación:
   ```
   rot +1d
   ```
   Salida esperada (aprox):
   ```
   time offset = +86400 sec
   rot +1d -> ROTATED
   INFO log: rotated midnight
   INFO log: file created name=log-20250920-091500.txt
   ```

3. **Listar para confirmar**:
   ```
   fs
   ```
   Debe aparecer el **nuevo archivo** con fecha diferente.

4. **Volver a tiempo normal**:
   ```
   rot reset
   ```

> 💡 Consejo: si querés probar múltiples rotaciones, repetí `rot +1d`, logeá algo (INFO/WARN), y listá con `fs`.

---

## 💧 2) Simular Low‑Water (poca memoria libre)

El mecanismo **low‑water** previene que el FS se quede sin espacio: si **free < (bytes_a_escribir + lowwater)**, el logger:
1. Cierra archivo actual.
2. **Borra** archivos de la **base path** (limpieza segura).
3. Abre un **archivo nuevo** y reintenta la escritura.
4. Vuelca el **boot buffer** si corresponde.

### Comando para ajustar low‑water

```
log lowwater <bytes>
```

### Cómo probar en caliente (paso a paso)

1. **Subir artificialmente el low‑water** por encima del espacio libre actual (ejemplo exagerado):  
   ```
   log lowwater 5000000
   ```

2. **Hacer una escritura** (con `log burst` escribís N líneas de tamaño `size`):
   ```
   log burst 1 16
   ```
   Esperá ver en la salida algo como:
   ```
   fs: wipe start
   fs: wipe done
   log: file created name=log-YYYYMMDD-HHMMSS.txt
   ```

3. **Volver a un valor razonable**:
   ```
   log lowwater 2048
   ```

4. **Ver estadística** del FS:
   ```
   fs stats
   ```
   Ejemplo:
   ```
   FS total=1478656 used=421888 free=1056768
   ```

> ⚠️ Importante: durante las pruebas con `log lowwater` muy alto, es **normal** que se borren archivos previos del FS. No pruebes en un FS con logs que quieras conservar.

---

## 🧪 Combinado: rotación + low‑water

Podés forzar un **cambio de día** y, a la vez, simular **espacio crítico** para verificar que ambas rutas convivan:

```
rot +1d
log lowwater 5000000
log burst 10 64
fs
log lowwater 2048
```

- Deberías ver **rotación** (nuevo archivo con fecha del “día siguiente”).
- Si el free < need + lowwater, se hará **wipe** + **archivo nuevo**.
- `fs` mostrará la lista actualizada.

---

## 🔧 Snippets útiles

### Forzar encabezado de nuevo archivo
```cpp
String header = String("VERSION=") + FW_VERSION +
                " MOTIVO_RESET=TEST_ROTATE IP=" + WiFi.localIP().toString();
Log.rotateDailyIfNeeded(header.c_str());
```

### Configurar low‑water por código
```cpp
Log.setFsLowWater(2048); // 2 KB por defecto en la demo
```

### Cambiar modo de salida para ver más claro
Durante pruebas de low‑water puede ser útil dejar solo FS o solo Serial:

```
out mode serial        // ver en consola sin escribir a FS
out mode log           // escribir en FS sin imprimir en Serial
out mode serial+log    // ambos (por defecto)
```

---

## ✅ Checklist visual (qué esperar)

- **Rotación**: aparece `log: rotated midnight` seguido de `log: file created name=...`, y `fs` muestra el nuevo archivo.
- **Low‑water**: aparecen `fs: wipe start` → `fs: wipe done` → `log: file created name=...` y la escritura continúa.
- **Errores**: si no hay FS montado o falla la apertura, verás `fs: mount FAIL` o `log: cannot create file serial-only` (se continúa por Serial).

---

## 🛡️ Recomendaciones

- Ejecutá las pruebas en **`cfg off`** (modo runtime) para que efectivamente se escriba en FS.  
  En **`cfg on`** las escrituras se pausan y van al **boot buffer**.
- Hacé `fs stats` **antes** y **después** de cada prueba para medir impacto.
- Volvé a un low‑water razonable (p.ej. **2048 B**) al finalizar.

---

## Referencia rápida de comandos

```
cfg on / cfg off
fs
fs stats
format
rot try | rot +1d | rot reset
log lowwater <bytes>
log burst N [size]
log level <TRACE|DEBUG|INFO|WARN|ERROR|CRIT>
out mode <off|serial|log|serial+log>
```

---

MIT · fs-logger-demo