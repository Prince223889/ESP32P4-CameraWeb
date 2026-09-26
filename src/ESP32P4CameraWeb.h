#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESP32P4Camera.h>

class ESP32P4CameraWeb {
public:
  ESP32P4CameraWeb() = default;

  bool begin(ESP32P4Camera &camera, const char *ssid, const char *password,
             uint8_t channel = 6, uint8_t maxConnections = 4);
  void loop();
  bool ready() const { return ready_; }
  IPAddress ip() const { return WiFi.softAPIP(); }
  uint16_t port() const { return port_; }
  bool startServer(uint16_t port = 80);

private:
  void handleRoot();
  void handleStatus();
  void handleSnapshot();
  void handleNotFound();
  void writeBmp(Client &client);
  static void put16(uint8_t *p, uint16_t v);
  static void put32(uint8_t *p, uint32_t v);

  ESP32P4Camera *camera_ = nullptr;
  WebServer server_{80};
  bool ready_ = false;
  uint16_t port_ = 80;
};

// Backward-compatible name for the earlier examples/API.
using P4CameraWeb = ESP32P4CameraWeb;
