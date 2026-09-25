#include "ESP32P4CameraWeb.h"

namespace {
const char kIndexHtml[] PROGMEM = R"HTML(
<!doctype html>
<html lang="en">
<head>
<meta name="viewport" content="width=device-width,initial-scale=1">
<meta charset="utf-8">
<title>ESP32-P4 Camera</title>
<style>
:root{color-scheme:dark}body{font-family:system-ui,sans-serif;background:#101418;color:#eef2f6;margin:0;padding:14px}
main{max-width:900px;margin:auto}.card{background:#181e24;border:1px solid #2b333c;border-radius:16px;padding:14px}
h1{font-size:1.15rem;margin:0 0 10px}img{display:block;width:100%;height:auto;min-height:160px;background:#000;border-radius:10px;object-fit:contain}
.row{display:flex;gap:8px;flex-wrap:wrap;margin-top:10px}button,select{border:0;border-radius:9px;padding:9px 12px;font:inherit}
.status{opacity:.8;font-size:.88rem;margin-top:8px;word-break:break-word}
</style>
</head>
<body>
<main><div class="card">
<h1>ESP32-P4 Camera</h1>
<img id="camera" alt="Camera frame">
<div class="row">
<button id="shot">Take photo</button>
<a id="save" href="/capture.bmp" download="p4-photo.bmp"><button type="button">Save image</button></a>
<label><input id="auto" type="checkbox"> Auto</label>
<select id="interval"><option value="1000" selected>1 s</option><option value="2000">2 s</option><option value="5000">5 s</option></select>
</div>
<div id="status" class="status">Connecting...</div>
</div></main>
<script>
const img=document.getElementById('camera');
const statusEl=document.getElementById('status');
const save=document.getElementById('save');
let timer=null;
function shot(){
  const url='/capture.bmp?t='+Date.now();
  statusEl.textContent='Capturing...';
  img.onload=()=>statusEl.textContent='Frame updated';
  img.onerror=()=>statusEl.textContent='Capture failed';
  img.src=url;
  save.href=url;
}
function arm(){
  if(timer){clearInterval(timer);timer=null;}
  if(document.getElementById('auto').checked){timer=setInterval(shot,Number(document.getElementById('interval').value));}
}
document.getElementById('shot').onclick=shot;
document.getElementById('auto').onchange=arm;
document.getElementById('interval').onchange=arm;
shot();
</script>
</body></html>
)HTML";
}

ESP32P4CameraWeb::ESP32P4CameraWeb(uint16_t port) : server_(port) {}

bool ESP32P4CameraWeb::begin(const char* ssid,
                             const char* password,
                             uint8_t i2cPort,
                             int8_t sclPin,
                             int8_t sdaPin,
                             uint32_t i2cFrequency,
                             size_t captureBuffers) {
  if (!camera_.begin(i2cPort, sclPin, sdaPin, i2cFrequency, captureBuffers)) {
    return false;
  }
  if (!startAP(ssid, password)) {
    camera_.stop();
    return false;
  }
  startServer();
  return true;
}

bool ESP32P4CameraWeb::startAP(const char* ssid, const char* password) {
  if (!ssid || !password || strlen(ssid) == 0 || strlen(ssid) > 32 || strlen(password) < 8) {
    return false;
  }
  WiFi.mode(WIFI_AP);
  WiFi.setSleep(false);
  const bool ok = WiFi.softAP(ssid, password);
  if (ok) {
    Serial.print("[WiFi] SSID: "); Serial.println(ssid);
    Serial.print("[WiFi] IP  : "); Serial.println(WiFi.softAPIP());
  }
  return ok;
}

void ESP32P4CameraWeb::startServer() {
  server_.on("/", HTTP_GET, [this]() { handleRoot(); });
  server_.on("/capture.bmp", HTTP_GET, [this]() { handleCapture(); });
  server_.on("/api/status", HTTP_GET, [this]() { handleStatus(); });
  server_.onNotFound([this]() { server_.send(404, "text/plain", "Not found"); });
  server_.begin();
  running_ = true;
  Serial.println("[HTTP] Server started on port 80");
}

void ESP32P4CameraWeb::handleClient() {
  if (running_) server_.handleClient();
}

void ESP32P4CameraWeb::handleRoot() {
  server_.send_P(200, "text/html; charset=utf-8", kIndexHtml);
}

void ESP32P4CameraWeb::handleStatus() {
  const auto& f = camera_.lastFrame();
  char json[256];
  snprintf(json, sizeof(json),
           "{\"ready\":%s,\"width\":%lu,\"height\":%lu,\"bytes\":%lu,\"format\":\"%s\"}",
           camera_.ready() ? "true" : "false",
           static_cast<unsigned long>(f.width),
           static_cast<unsigned long>(f.height),
           static_cast<unsigned long>(f.size),
           f.formatName);
  server_.send(200, "application/json", json);
}

void ESP32P4CameraWeb::handleCapture() {
  // The exact BMP size is known after camera capture, but WebServer's public
  // client is a Print-like stream. We deliberately use an unknown-length
  // response so the camera can stream the BMP directly and avoid a second
  // full-frame copy in RAM.
  server_.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server_.send(200, "image/bmp", "");

  if (!serveBmp(server_.client())) {
    // The HTTP header has already been sent, so only serial diagnostics are safe here.
    Serial.println("[CameraWeb] BMP capture failed");
  }
}

bool ESP32P4CameraWeb::serveBmp(NetworkClient& client) {
  ESP32P4Camera::FrameInfo info;
  const bool ok = camera_.writeBMP(client, &info);
  if (ok) {
    Serial.print("[CameraWeb] BMP: ");
    Serial.print(info.width); Serial.print('x'); Serial.print(info.height);
    Serial.print(" / "); Serial.print((unsigned long)info.size); Serial.println(" bytes RGB565 input");
  }
  return ok;
}
