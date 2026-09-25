#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESP32P4Camera.h>

class ESP32P4CameraWeb {
public:
  explicit ESP32P4CameraWeb(uint16_t port = 80);

  bool begin(const char* ssid = "ESP32-P4-Camera",
             const char* password = "12345678",
             uint8_t i2cPort = 0,
             int8_t sclPin = 8,
             int8_t sdaPin = 7,
             uint32_t i2cFrequency = 400000,
             size_t captureBuffers = 2);

  bool startAP(const char* ssid, const char* password);
  void startServer();
  void handleClient();

  bool running() const { return running_; }
  ESP32P4Camera& camera() { return camera_; }
  const ESP32P4Camera& camera() const { return camera_; }

private:
  void handleRoot();
  void handleCapture();
  void handleStatus();
  bool serveBmp(NetworkClient& client);

  ESP32P4Camera camera_;
  WebServer server_;
  bool running_ = false;
};

// Compatibility aliases: both spellings compile.
using P4CameraWeb = ESP32P4CameraWeb;
