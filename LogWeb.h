#ifndef LOGWEB_H
#define LOGWEB_H

#include <Arduino.h>
#include <LittleFS.h>
#include <WebServer.h>

class LogWeb {
public:
  LogWeb(uint16_t port = 80, const char* basePath = "/");

  void begin();
  void loop();

  void enterWebMode();
  void exitWebMode();
  bool inWebMode() const { return webMode_; }

  void setBasePath(const String& base);

private:
  void setupEndpoints();
  String urlDecode(const String& src);
  String sanitizePath(const String& raw, const String& base);
  String renderDirHTML(const String& dirPath);

  uint16_t   port_;
  String     basePath_;
  WebServer  server_;
  bool       webMode_;
};

#endif // LOGWEB_H
