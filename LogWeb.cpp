#include "LogWeb.h"
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
// ctor
// ─────────────────────────────────────────────────────────────────────────────
LogWeb::LogWeb(uint16_t port, const char* basePath)
: port_(port ? port : 80),
  basePath_(basePath ? basePath : "/"),
  server_(port_),
  webMode_(false)
{
  if (!basePath_.startsWith("/")) basePath_ = "/" + basePath_;
  if (!basePath_.endsWith("/"))   basePath_ += "/";
}

// ─────────────────────────────────────────────────────────────────────────────
// público
// ─────────────────────────────────────────────────────────────────────────────
void LogWeb::begin() {
  setupEndpoints();
  server_.begin();
}

void LogWeb::loop() {
  if (webMode_) {
    server_.handleClient();
    delay(1);
  }
}

void LogWeb::enterWebMode() {
  webMode_ = true;
}

void LogWeb::exitWebMode() {
  webMode_ = false;
}

void LogWeb::setBasePath(const String& base) {
  if (base.isEmpty()) return;
  basePath_ = base;
  if (!basePath_.startsWith("/")) basePath_ = "/" + basePath_;
  if (!basePath_.endsWith("/"))   basePath_ += "/";
}

// ─────────────────────────────────────────────────────────────────────────────
// endpoints
// ─────────────────────────────────────────────────────────────────────────────
void LogWeb::setupEndpoints() {
  // raíz → /fs
  server_.on("/", HTTP_GET, [this](){
    server_.sendHeader("Location", "/fs");
    server_.send(302);
  });

  // borrar todos los archivos
  server_.on("/fs/erase", HTTP_GET, [this](){
    File root = LittleFS.open(basePath_, FILE_READ);
    if (!root || !root.isDirectory()) {
      server_.send(500, "text/plain", "directorio inválido");
      return;
    }

    std::vector<String> names;
    for (File f = root.openNextFile(); f; f = root.openNextFile()) {
      if (!f.isDirectory()) names.push_back(f.name());
    }
    root.close();

    size_t ok = 0, fail = 0;
    for (const auto& n : names) {
      bool removed = LittleFS.remove(n);
      removed ? ok++ : fail++;
    }

    server_.sendHeader("Location", "/fs");
    server_.send(302, "text/plain",
                 String("logs borrados ok=") + ok + " fail=" + fail);
  });

  // formatear TODO
  server_.on("/fs/format", HTTP_GET, [this](){
    bool ok = LittleFS.format();
    server_.sendHeader("Location", "/fs");
    server_.send(302, "text/plain", ok ? "FS formateado" : "Error formateando FS");
  });

  // listar
  server_.on("/fs", HTTP_GET, [this](){
    String path = basePath_;
    if (server_.hasArg("path")) {
      path = sanitizePath(server_.arg("path"), basePath_);
    }
    server_.send(200, "text/html; charset=utf-8", renderDirHTML(path));
  });

  // ver archivo
  server_.on("/fs/view", HTTP_GET, [this](){
    if (!server_.hasArg("path")) { server_.send(400, "text/plain", "missing path"); return; }
    String path = sanitizePath(server_.arg("path"), basePath_);
    if (!LittleFS.exists(path))  { server_.send(404, "text/plain", "not found");   return; }
    File f = LittleFS.open(path, FILE_READ);
    if (!f) { server_.send(500, "text/plain", "open fail"); return; }
    server_.streamFile(f, "text/plain; charset=utf-8");
    f.close();
  });

  // descargar archivo
  server_.on("/fs/download", HTTP_GET, [this](){
    if (!server_.hasArg("path")) { server_.send(400, "text/plain", "missing path"); return; }
    String path = sanitizePath(server_.arg("path"), basePath_);
    if (!LittleFS.exists(path))  { server_.send(404, "text/plain", "not found");   return; }
    File f = LittleFS.open(path, FILE_READ);
    if (!f) { server_.send(500, "text/plain", "open fail"); return; }

    String name = path.substring(path.lastIndexOf('/') + 1);
    server_.setContentLength(f.size());
    server_.sendHeader("Content-Disposition", "attachment; filename=" + name);
    server_.send(200, "application/octet-stream", "");
    uint8_t buf[1024];
    while (int n = f.read(buf, sizeof(buf))) {
      server_.client().write(buf, n);
      delay(0);
    }
    f.close();
  });
}

// ─────────────────────────────────────────────────────────────────────────────
// helpers
// ─────────────────────────────────────────────────────────────────────────────
String LogWeb::urlDecode(const String& src){
  String dst; dst.reserve(src.length());
  for (uint16_t i=0; i<src.length(); ++i){
    char c = src[i];
    if (c=='%' && i+2 < src.length()){
      char h1 = src[i+1], h2 = src[i+2];
      auto hex = [](char h)->int{
        if(h>='0'&&h<='9') return h-'0';
        if(h>='A'&&h<='F') return 10+(h-'A');
        if(h>='a'&&h<='f') return 10+(h-'a');
        return -1;
      };
      int v1 = hex(h1), v2 = hex(h2);
      if (v1>=0 && v2>=0){ dst += char(v1*16+v2); i+=2; continue; }
    }
    if (c=='+') dst+=' '; else dst+=c;
  }
  return dst;
}

String LogWeb::sanitizePath(const String& raw, const String& base){
  String p = urlDecode(raw);
  String b = base;
  if (!b.startsWith("/")) b = "/" + b;
  if (!b.endsWith("/"))   b += "/";

  if (p.isEmpty()) p = b;
  if (!p.startsWith("/")) p = "/" + p;
  p.replace("..","");

  if (!p.startsWith(b)) {
    String tail = p;
    if (tail.startsWith("/")) tail.remove(0,1);
    p = b + tail;
  }
  return p;
}

String LogWeb::renderDirHTML(const String& dirPath){
  String html;
  html += F("<!doctype html><meta charset='utf-8'><title>Logs</title>");
  html += F("<style>body{font-family:system-ui,Arial;margin:16px} ul{line-height:1.8}</style>");
  html += F("<h2>Archivos en ");
  html += dirPath;
  html += F("</h2><ul>");

  File dir = LittleFS.open(dirPath, FILE_READ);
  if (!dir || !dir.isDirectory()){
    html += F("<li><b>Directorio inválido</b></li></ul>");
    return html;
  }

  for (File f = dir.openNextFile(); f; f = dir.openNextFile()){
    String name = f.name();
    if (f.isDirectory()){
      html += "<li>[DIR] <a href='/fs?path=" + name + "/'>" + name + "/</a></li>";
    } else {
      html += "<li><a href='/fs/download?path=" + name + "'>" + name + "</a>";
      html += " (" + String((unsigned)f.size()) + " B)";
      html += " — <a href='/fs/view?path=" + name + "'>ver</a>";
      html += "</li>";
    }
  }
  html += "</ul>";    
  html += "<p>"
          "<a href='/fs/erase' onclick=\"return confirm('¿Borrar todos los logs?');\">🗑️ Borrar todos los logs</a>"
          " &nbsp;|&nbsp; "
          "<a href='/fs/format' onclick=\"return confirm('⚠️ FORMATEAR LittleFS COMPLETO. Esto borra TODO y no se puede deshacer. ¿Continuar?');\">🧨 Formatear LittleFS</a>"
          "</p>";

  return html;
}
