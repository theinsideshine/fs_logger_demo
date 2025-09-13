#ifndef CLOGFS_H
#define CLOGFS_H

#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>

class ClogFS {
public:
  ClogFS();

  // Config
  void setTimeProvider(time_t (*nowFn)());
  void setBootBufferCapacityBytes(size_t cap);
  void setFsLowWater(size_t bytes);      // margen para wipe preventivo (FS bajo)
  void setBasePath(const char* p);       // base para listar/borrar (default "/")

  // Archivo
  bool openFile(const char* filename, const char* header_ascii = nullptr);
  bool rotate(const String& newFilename, const char* header_ascii = nullptr);
  bool rotateDailyIfNeeded(const char* header_ascii = nullptr);
  void closeFile();

  // Log
  void msg(const __FlashStringHelper *fmt, ...);
  void ctrl(uint16_t raw, uint16_t filtered, uint8_t state, uint16_t danger_point);
  void log_sensor_with_state(const char* name, float value, float min, float max,
                             int state, float state_scale, const char* state_label);

  // Utilidad
  void listDir(const char* path = "/");
  static String makeFilename(time_t epoch);
  void resetDayTracking();

private:
  void writeLineASCII(const char* line);
  void flushBootBufferToFile();
  void bootBufAppendLine(const char* line);
  bool buildTimePrefix(char* ts, size_t ts_len);

  // FS helpers
  size_t fsFreeBytes() const;
  bool   wipeAllInBasePath();
  bool   reopenFreshFileAfterWipe(const char* header_ascii);

  // estado
  int    _lastDay;           // día de mes de la última escritura/rotación
  File   _file;
  bool   _fileOk;
  bool   _fileReady;
  time_t (*_nowFn)();        // proveedor de hora

  String _bootBuf;
  size_t _bootCap;

  // FS full control
  size_t _fsLowWater;        // margen de seguridad (bytes)
  String _basePath;          // ruta base para borrar/listar
  String _currentPath;       // último archivo abierto (informativo)
};

#endif // CLOGFS_H





