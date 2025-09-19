// ClogFS.h  (REEMPLAZO COMPLETO; sin auto-include)
#ifndef CLOG_FS_H
#define CLOG_FS_H

#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>

class ClogFS {
public:
  enum Severity : uint8_t { TRACE=0, DEBUG=1, INFO=2, WARN=3, ERROR=4, CRIT=5 };

  enum Level : uint8_t {
  LVL_OFF            = 0,
  LVL_SERIAL         = 1,
  LVL_LOG_ONLY       = 2,
  LVL_SERIAL_AND_LOG = 3
};


  ClogFS();

  // config
  void setTimeProvider(time_t (*nowFn)());
  void setBootBufferCapacityBytes(size_t cap);
  void setFsLowWater(size_t bytes);
  void setBasePath(const char* p);

  // severidad (umbral)
  void setMinSeverity(Severity s);
  Severity minSeverity() const { return _minSev; }
  static const char* sevName(Severity s);

  // open/rotate/close
  bool openFile(const char* filename, const char* header_ascii=nullptr);
  bool rotate(const String& newFilename, const char* header_ascii=nullptr);
  bool rotateDailyIfNeeded(const char* header_ascii=nullptr);
  void closeFile();
  void resetDayTracking();

  // API de log (msg == info)
  void msg  (const __FlashStringHelper *fmt, ...);
  void trace(const __FlashStringHelper *fmt, ...);
  void debug(const __FlashStringHelper *fmt, ...);
  void info (const __FlashStringHelper *fmt, ...);
  void warn (const __FlashStringHelper *fmt, ...);
  void error(const __FlashStringHelper *fmt, ...);
  void crit (const __FlashStringHelper *fmt, ...);

  // utilidades 
  void listDir(const char* path);
  static String makeFilename(time_t epoch);

  // FS
  size_t fsFreeBytes() const;
  bool wipeAllInBasePath();
  bool reopenFreshFileAfterWipe(const char* header_ascii=nullptr);

  void setLevel(Level lv) { _level = lv; }
  Level level() const { return _level; }

private:
  void vmsg_(Severity sev, const __FlashStringHelper *fmt, va_list ap);
  void writeLineASCII(const char* line);
  void flushBootBufferToFile();
  void bootBufAppendLine(const char* line);
  bool buildTimePrefix(char* ts, size_t ts_len);

  File   _file;
  bool   _fileOk, _fileReady;
  time_t (*_nowFn)();
  String _bootBuf;
  size_t _bootCap;
  int    _lastDay;
  size_t _fsLowWater;
  String _basePath;
  String _currentPath;
  Severity _minSev;

  Level _level;

};

#endif // CLOG_FS_H


