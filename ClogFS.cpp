#include "ClogFS.h"
#include <time.h>

// C++/libc headers usados en este .cpp
#include <cstdarg>   // va_list, va_start, va_end
#include <cstdio>    // snprintf
#include <cstring>   // strlen

// --- ctor ---
ClogFS::ClogFS()
: _file(), _fileOk(false), _fileReady(false),
  _nowFn(nullptr),
  _bootBuf(), _bootCap(4096),
  _lastDay(-1),
  _fsLowWater(2048),
  _basePath("/")
{}

// --- config ---
void ClogFS::setTimeProvider(time_t (*nowFn)()){ _nowFn = nowFn; }
void ClogFS::setBootBufferCapacityBytes(size_t cap){ _bootCap = cap ? cap : 1024; }
void ClogFS::setFsLowWater(size_t bytes){ _fsLowWater = bytes; }
void ClogFS::setBasePath(const char* p){ _basePath = (p && *p) ? String(p) : String("/"); }

// --- open/rotate/close ---
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

// --- msg ---
void ClogFS::msg(const __FlashStringHelper *fmt, ...){
  char buf[192];
  va_list args;
  va_start(args, fmt);
  vsnprintf_P(buf, sizeof(buf), (PGM_P)fmt, args);
  va_end(args);

  char ts[12] = {0};
  bool have_ts = buildTimePrefix(ts, sizeof(ts));

  if (have_ts) {
    Serial.print(ts);
  } else {
    Serial.print(millis());
    Serial.print(" ");
  }
  Serial.println(buf);

  writeLineASCII(buf);
}

// --- helpers varios ---
void ClogFS::ctrl(uint16_t raw, uint16_t filtered, uint8_t state, uint16_t danger_point){
  Serial.printf("%lu ctrl: raw=%u filtered=%u state=%u danger=%u\n",
                (unsigned long)millis(), raw, filtered, state, danger_point);
  char line[128];
  snprintf(line, sizeof(line), "ctrl raw=%u filtered=%u state=%u danger=%u",
           raw, filtered, state, danger_point);
  writeLineASCII(line);
}

void ClogFS::log_sensor_with_state(const char* name, float value, float min, float max,
                                   int state, float, const char*){
  Serial.printf("%lu sensor: %s=%.2f min=%.2f max=%.2f state=%d\n",
                (unsigned long)millis(), name, value, min, max, state);
  char line[160];
  snprintf(line, sizeof(line), "%s=%.2f %s_min=%.2f %s_max=%.2f %s_state=%d",
           name, value, name, min, name, max, name, state);
  writeLineASCII(line);
}

void ClogFS::listDir(const char* path){
  const char* p = (path && *path) ? path : "/";
  File dir = LittleFS.open(p, FILE_READ);
  if (!dir || !dir.isDirectory()){
    writeLineASCII("fs: cannot open dir");
    return;
  }
  File f = dir.openNextFile();
  if (!f) {
    writeLineASCII("fs: directory empty");
    dir.close();
    return;
  }
  while (f) {
    String name = f.name();
    unsigned sz = (unsigned) f.size();
    f.close();
    char line[160];
    snprintf(line, sizeof(line), "fs: found %s (%uB)", name.c_str(), sz);
    writeLineASCII(line);
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

// --- FS lleno ---
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

    // Construir ruta completa segura:
    String full = name;
    if (!full.startsWith("/")) {
      if (_basePath.endsWith("/")) full = _basePath + full;
      else                         full = _basePath + "/" + full;
    }

    f.close();

    if (!isDir) {
      // intentar con 'full'; si por algún motivo falla, intentar con 'name' tal cual
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
  char ts[12] = {0};
  bool have_ts = buildTimePrefix(ts, sizeof(ts));
  size_t need = strlen(line) + 1 + (have_ts ? strlen(ts) : 0);

  size_t freeB = fsFreeBytes();
if (freeB > 0 && freeB < (need + _fsLowWater)) {
  closeFile();                           // <— asegurar que el actual no esté abierto
  wipeAllInBasePath();
  reopenFreshFileAfterWipe("FS_WIPE=1");
}



  bool ok1 = false;
  if (_fileReady && _fileOk) {
    if (have_ts) _file.print(ts);
    size_t n = _file.println(line);
    ok1 = (n > 0);
  } else {
    if (have_ts){
      String tmp; tmp.reserve(strlen(line) + 12);
      tmp += ts; tmp += line;
      bootBufAppendLine(tmp.c_str());
    } else {
      bootBufAppendLine(line);
    }
    return;
  }

  if (ok1) return;

  // Fallback: fallo de escritura
  wipeAllInBasePath();
  if (reopenFreshFileAfterWipe("FS_WIPE=1")) {
    if (have_ts) _file.print(ts);
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

