#include "ESP32P4CameraWeb.h"

static const char *kPage = R"HTML(<!doctype html>
<html><head><meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESP32-P4 Camera</title>
<style>body{font-family:system-ui;margin:0;background:#111;color:#eee}main{max-width:920px;margin:auto;padding:14px}img{width:100%;height:auto;display:block;background:#000;border-radius:12px}button{padding:10px 14px;margin:8px 6px 8px 0;border:0;border-radius:9px;font-weight:600}pre{background:#1d1d1d;padding:10px;border-radius:9px;overflow:auto}</style></head>
<body><main><h2>ESP32-P4 Camera</h2><img id="cam" alt="camera">
<div><button onclick="snap()">Take photo</button><button onclick="toggle()" id="auto">Auto: ON</button></div>
<pre id="status">loading...</pre>
<script>
let timer=null, auto=true;
function snap(){document.getElementById('cam').src='/snapshot.bmp?t='+Date.now();}
function tick(){if(auto)snap();}
function toggle(){auto=!auto;document.getElementById('auto').textContent='Auto: '+(auto?'ON':'OFF');}
async function info(){try{let r=await fetch('/status');document.getElementById('status').textContent=await r.text();}catch(e){document.getElementById('status').textContent=e}}
snap();info();timer=setInterval(tick,1500);
</script></main></body></html>)HTML";

bool ESP32P4CameraWeb::begin(ESP32P4Camera &camera, const char *ssid, const char *password,
                              uint8_t channel, uint8_t maxConnections) {
  camera_ = &camera;
  if (!camera_->ready()) {
    Serial.println("[ESP32P4-CameraWeb] Camera must be initialized first.");
    return false;
  }
  WiFi.mode(WIFI_AP);
  if (!WiFi.softAP(ssid, password, channel, 0, maxConnections)) {
    Serial.println("[ESP32P4-CameraWeb] SoftAP start failed.");
    return false;
  }
  return startServer(80);
}

bool ESP32P4CameraWeb::startServer(uint16_t port) {
  port_ = port;
  if (port != 80) {
    // WebServer's listening port is selected by its constructor. Keep the public API
    // simple and require the standard port for the current preview implementation.
    Serial.println("[ESP32P4-CameraWeb] This preview uses HTTP port 80.");
    return false;
  }
  server_.on("/", HTTP_GET, [this]() { handleRoot(); });
  server_.on("/status", HTTP_GET, [this]() { handleStatus(); });
  server_.on("/snapshot.bmp", HTTP_GET, [this]() { handleSnapshot(); });
  server_.onNotFound([this]() { handleNotFound(); });
  server_.begin();
  ready_ = true;
  Serial.print("CAMERA_WEB_IP=");
  Serial.println(WiFi.softAPIP().toString());
  return true;
}

void ESP32P4CameraWeb::loop() {
  if (ready_) server_.handleClient();
}

void ESP32P4CameraWeb::handleRoot() {
  server_.sendHeader("Cache-Control", "no-store");
  server_.send(200, "text/html; charset=utf-8", kPage);
}

void ESP32P4CameraWeb::handleStatus() {
  String json = "{\"ip\":\"" + WiFi.softAPIP().toString() +
                "\",\"width\":" + String((unsigned long)camera_->width()) +
                ",\"height\":" + String((unsigned long)camera_->height()) +
                ",\"frameBytes\":" + String((unsigned long)camera_->size()) + "}";
  server_.sendHeader("Cache-Control", "no-store");
  server_.send(200, "application/json", json);
}

void ESP32P4CameraWeb::handleSnapshot() {
  if (!camera_->capture()) {
    server_.send(503, "text/plain", "camera capture failed");
    return;
  }
  if (camera_->width() == 0 || camera_->height() == 0 || !camera_->data()) {
    server_.send(500, "text/plain", "invalid camera frame");
    return;
  }
  const uint32_t rowBytes = camera_->width() * 3u;
  const uint32_t rowStride = (rowBytes + 3u) & ~3u;
  const uint32_t imageBytes = rowStride * camera_->height();
  const uint32_t total = 54u + imageBytes;
  server_.sendHeader("Cache-Control", "no-store");
  server_.setContentLength(total);
  server_.send(200, "image/bmp", "");
  Client &client = server_.client();
  writeBmp(client);
}

void ESP32P4CameraWeb::put16(uint8_t *p, uint16_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }
void ESP32P4CameraWeb::put32(uint8_t *p, uint32_t v) { p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8); p[2]=(uint8_t)(v>>16); p[3]=(uint8_t)(v>>24); }

void ESP32P4CameraWeb::writeBmp(Client &client) {
  const uint32_t w = camera_->width();
  const uint32_t h = camera_->height();
  const uint32_t rowBytes = w * 3u;
  const uint32_t rowStride = (rowBytes + 3u) & ~3u;
  const uint32_t imageBytes = rowStride * h;
  uint8_t header[54] = {};
  header[0] = 'B'; header[1] = 'M';
  put32(header + 2, 54u + imageBytes);
  put32(header + 10, 54u);
  put32(header + 14, 40u);
  put32(header + 18, w);
  put32(header + 22, h);
  put16(header + 26, 1);
  put16(header + 28, 24);
  put32(header + 34, imageBytes);
  put32(header + 38, 2835u);
  put32(header + 42, 2835u);
  client.write(header, sizeof(header));

  const uint8_t *src = camera_->data();
  uint8_t *row = static_cast<uint8_t*>(malloc(rowStride));
  if (!row) return;
  for (int32_t y = (int32_t)h - 1; y >= 0; --y) {
    const uint8_t *line = src + ((size_t)y * w * 2u);
    for (uint32_t x = 0; x < w; ++x) {
      const size_t si = (size_t)x * 2u;
      uint16_t px = (uint16_t)line[si] | ((uint16_t)line[si+1] << 8);
      row[x*3u + 0] = (uint8_t)((px & 0x1F) * 255 / 31);          // B
      row[x*3u + 1] = (uint8_t)(((px >> 5) & 0x3F) * 255 / 63);  // G
      row[x*3u + 2] = (uint8_t)(((px >> 11) & 0x1F) * 255 / 31); // R
    }
    if (rowStride > rowBytes) memset(row + rowBytes, 0, rowStride - rowBytes);
    client.write(row, rowStride);
  }
  free(row);
}

void ESP32P4CameraWeb::handleNotFound() {
  server_.sendHeader("Location", "/", true);
  server_.send(302, "text/plain", "redirect");
}
