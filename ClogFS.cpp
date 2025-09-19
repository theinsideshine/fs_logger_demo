#include "ClogFS.h"
#include <time.h>
#include <cstdarg>
#include <cstdio>
#include <cstring>

ClogFS::ClogFS()
: _file(), _fileOk(false), _fileReady(false),
  _nowFn(nullptr),
  _bootBuf(), _bootCap(4096),
  _lastDay(-1),
  _fsLowWater(2048),
  _basePath("/"),
  _currentPath(""),
  _minSev(INFO),
  _level(LVL_SERIAL_AND_LOG)   // default: Serial + FS
{}


// config
void ClogFS::setTimeProvider(time_t (*nowFn)()){ _nowFn = nowFn; }
void ClogFS::setBootBufferCapacityBytes(size_t cap){ _bootCap = cap ? cap : 1024; }
void ClogFS::setFsLowWater(size_t bytes){ _fsLowWater = bytes; }
void ClogFS::setBasePath(const char* p){ _basePath = (p && *p) ? String(p) : String("/"); }

// severidad helpers
const char* ClogFS::sevName(Severity s){
  switch(s){
    case TRACE: return "TRACE";
    case DEBUG: return "DEBUG";
    case INFO:  return "INFO";
    case WARN:  return "WARN";
    case ERROR: return "ERROR";
    case CRIT:  return "CRIT";
  }
  return "?";
}
void ClogFS::setMinSeverity(Severity s){ _minSev = s; }

// open/rotate/close
bool ClogFS::openFile(const char* filename, const char* header_ascii){
  if (_file) { _file.flush(); _file.close(); }

  String full = (filename && filename[0] == '/') ? String(filename)
                                                 : (String("/") + filename);
  if (full[0] != '/') full = String("/") + full;

  _file = LittleFS.open(full.c_str(), FILE_WRITE);
  _fileOk = _file && _file.print("") >= 0;
  _fileReady = _fileOk;
  if (_fileOk) _currentPath = full;

  if (_fileOk && header_ascii && *header_ascii) {
    _file.println(header_ascii);
    _file.flush();
  }
  if (_fileOk) {
    flushBootBufferToFile();
  }

  if (_nowFn) {
    time_t t = _nowFn();
    if (t > 0) {
      struct tm* tm_info = localtime(&t);
      if (tm_info) _lastDay = tm_info->tm_mday;
    }
  }
  return _fileOk;
}

bool ClogFS::rotate(const String& newFilename, const char* header_ascii){
  if (_file) { _file.flush(); _file.close(); }
  _fileReady = false;

  String full = newFilename.startsWith("/") ? newFilename : (String("/") + newFilename);
  _file = LittleFS.open(full.c_str(), FILE_WRITE);
  _fileOk = _file && _file.print("") >= 0;
  _fileReady = _fileOk;
  if (_fileOk) _currentPath = full;

  if (_fileOk && header_ascii && *header_ascii) {
    _file.println(header_ascii);
    _file.flush();
  }

  if (_fileOk && _nowFn) {
    time_t t = _nowFn();
    if (t > 0) {
      struct tm* tm_info = localtime(&t);
      if (tm_info) _lastDay = tm_info->tm_mday;
    }
  }
  return _fileOk;
}

bool ClogFS::rotateDailyIfNeeded(const char* header_ascii){
  if (!_nowFn) return false;
  time_t now = _nowFn();
  if (now <= 0) return false;
  struct tm* tm_info = localtime(&now);
  if (!tm_info) return false;

  int today = tm_info->tm_mday;
  if (_lastDay == -1) { _lastDay = today; return false; }
  if (today == _lastDay) return false;

  String newName = ClogFS::makeFilename(now);
  bool ok = rotate(newName, header_ascii);
  if (ok) _lastDay = today;
  return ok;
}

void ClogFS::closeFile(){
  if (_file) { _file.flush(); _file.close(); }
  _fileOk = false;
  _fileReady = false;
}

// API de log (msg == info)
void ClogFS::msg(const __FlashStringHelper *fmt, ...){
  va_list ap; va_start(ap, fmt); vmsg_(INFO, fmt, ap); va_end(ap);
}
void ClogFS::trace(const __FlashStringHelper *fmt, ...){
  va_list ap; va_start(ap, fmt); vmsg_(TRACE, fmt, ap); va_end(ap);
}
void ClogFS::debug(const __FlashStringHelper *fmt, ...){
  va_list ap; va_start(ap, fmt); vmsg_(DEBUG, fmt, ap); va_end(ap);
}
void ClogFS::info(const __FlashStringHelper *fmt, ...){
  va_list ap; va_start(ap, fmt); vmsg_(INFO, fmt, ap); va_end(ap);
}
void ClogFS::warn(const __FlashStringHelper *fmt, ...){
  va_list ap; va_start(ap, fmt); vmsg_(WARN, fmt, ap); va_end(ap);
}
void ClogFS::error(const __FlashStringHelper *fmt, ...){
  va_list ap; va_start(ap, fmt); vmsg_(ERROR, fmt, ap); va_end(ap);
}
void ClogFS::crit(const __FlashStringHelper *fmt, ...){
  va_list ap; va_start(ap, fmt); vmsg_(CRIT, fmt, ap); va_end(ap);
}

void ClogFS::vmsg_(Severity sev, const __FlashStringHelper *fmt, va_list ap){
  if (sev < _minSev) return;
  if (_level == LVL_OFF) return;

  char buf[192];
  vsnprintf_P(buf, sizeof(buf), (PGM_P)fmt, ap);

  char ts[16] = {0};
  bool have_ts = buildTimePrefix(ts, sizeof(ts));

  char line[224];
  if (have_ts) snprintf(line, sizeof(line), "%s %s%s", sevName(sev), ts, buf);
  else         snprintf(line, sizeof(line), "%s %lu %s", sevName(sev), (unsigned long)millis(), buf);

  // Serial?
  if (_level == LVL_SERIAL || _level == LVL_SERIAL_AND_LOG) {
    Serial.println(line);
  }
  // FS?
  if (_level == LVL_LOG_ONLY || _level == LVL_SERIAL_AND_LOG) {
    writeLineASCII(line);
  }
}


// utilidades


void ClogFS::listDir(const char* path){
  const char* p = (path && *path) ? path : "/";
  File dir = LittleFS.open(p, FILE_READ);
  if (!dir || !dir.isDirectory()){
    // problema al abrir -> WARN
    warn(F("fs: cannot open dir"));
    return;
  }
  File f = dir.openNextFile();
  if (!f) {
    // sin archivos -> DEBUG
    debug(F("fs: directory empty"));
    dir.close();
    return;
  }
  while (f) {
    String name = f.name();
    unsigned sz = (unsigned) f.size();
    f.close();
    // listado de archivos -> DEBUG
    debug(F("fs: found %s (%uB)"), name.c_str(), sz);
    f = dir.openNextFile();
  }
  dir.close();
}


String ClogFS::makeFilename(time_t epoch){
  struct tm* tm_info = localtime(&epoch);
  char name[32];
  snprintf(name, sizeof(name), "log-%04d%02d%02d-%02d%02d%02d.txt",
           tm_info->tm_year+1900, tm_info->tm_mon+1, tm_info->tm_mday,
           tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
  return String(name);
}

// FS
size_t ClogFS::fsFreeBytes() const {
#if defined(ARDUINO_ARCH_ESP32)
  size_t total = LittleFS.totalBytes();
  size_t used  = LittleFS.usedBytes();
  return (total >= used) ? (total - used) : 0;
#else
  return 0;
#endif
}

bool ClogFS::wipeAllInBasePath(){
  File dir = LittleFS.open(_basePath.c_str(), FILE_READ);
  if (!dir || !dir.isDirectory()) return false;

  for (File f = dir.openNextFile(); f; f = dir.openNextFile()){
    String name = f.name();
    bool isDir = f.isDirectory();
    String full = name;
    if (!full.startsWith("/")) {
      if (_basePath.endsWith("/")) full = _basePath + full;
      else                         full = _basePath + "/" + full;
    }
    f.close();
    if (!isDir) {
      if (!LittleFS.remove(full)) {
        LittleFS.remove(name);
      }
    }
  }
  dir.close();
  return true;
}

bool ClogFS::reopenFreshFileAfterWipe(const char* header_ascii){
  closeFile();
  time_t now = _nowFn ? _nowFn() : time(nullptr);
  if (now <= 0) now = time(nullptr);
  String newName = makeFilename(now);
  return openFile(newName.c_str(), header_ascii);
}

void ClogFS::writeLineASCII(const char* line){
  // si no estamos en un modo que escribe a FS, salgo
  if (!(_level == LVL_LOG_ONLY || _level == LVL_SERIAL_AND_LOG)) return;

  size_t need = strlen(line) + 1;

  size_t freeB = fsFreeBytes();
  if (freeB > 0 && freeB < (need + _fsLowWater)) {
    closeFile();
    wipeAllInBasePath();
    reopenFreshFileAfterWipe("FS_WIPE=1");
  }

  bool ok1 = false;
  if (_fileReady && _fileOk) {
    size_t n = _file.println(line);
    ok1 = (n > 0);
  } else {
    bootBufAppendLine(line);
    return;
  }

  if (ok1) return;

  // Fallback
  wipeAllInBasePath();
  if (reopenFreshFileAfterWipe("FS_WIPE=1")) {
    _file.println(line);
  }
}


void ClogFS::flushBootBufferToFile(){
  if(!_fileOk || _bootBuf.isEmpty()) return;
  _file.print(_bootBuf);
  _file.flush();
  _bootBuf = "";
}

void ClogFS::bootBufAppendLine(const char* line){
  if(!line) return;
  size_t needed = _bootBuf.length() + strlen(line) + 1;
  if(needed > _bootCap){
    size_t overflow = needed - _bootCap;
    size_t cut = 0;
    while(overflow > 0 && cut < _bootBuf.length()){
      int nl = _bootBuf.indexOf('\n', cut);
      if(nl < 0){ _bootBuf = ""; break; }
      size_t chunk = (nl + 1) - cut;
      cut = nl + 1;
      if(chunk >= overflow){ _bootBuf.remove(0, cut); break; }
      overflow -= chunk;
      if(cut >= _bootBuf.length()){ _bootBuf = ""; break; }
    }
  }
  _bootBuf += line;
  _bootBuf += '\n';
}

bool ClogFS::buildTimePrefix(char* ts, size_t ts_len){
  if(!_nowFn) return false;
  time_t t = _nowFn();
  if(t <= 0) return false;
  struct tm* tm_info = localtime(&t);
  if(!tm_info) return false;
  snprintf(ts, ts_len, "%02d:%02d:%02d ", tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
  return true;
}

void ClogFS::resetDayTracking(){ _lastDay = -1; }



