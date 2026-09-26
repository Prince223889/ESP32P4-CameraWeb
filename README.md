# ESP32P4-CameraWeb

A deliberately small ESP32-P4 camera web server. It creates a Wi-Fi SoftAP through the board's Wi-Fi subsystem, serves one light HTML page, and refreshes only the `<img>` element. It does **not** reload the whole page every time a photo is requested.

## Hardware

Designed for Waveshare ESP32-P4-WIFI6-DEV-KIT + OV5647.

## Usage

```cpp
#include <ESP32P4Camera.h>
#include <ESP32P4CameraWeb.h>

ESP32P4Camera camera;
ESP32P4CameraWeb web;

void setup() {
  Serial.begin(115200);
  camera.begin();
  web.begin(camera, "ESP32P4-Camera", "12345678");
}
void loop() { web.loop(); }
```

The serial monitor prints `CAMERA_WEB_IP=...`. Connect your phone to the AP and open that IP.

## Why BMP in the first release?

The first release uses a known-length BMP response, which avoids HTTP/1.1 chunking problems and keeps the implementation dependency-free. A future release can add the ESP32-P4 hardware JPEG encoder for much smaller images; Espressif exposes a dedicated JPEG encoder video device on P4.

## HTTP endpoints

- `/` page
- `/status` JSON status
- `/snapshot.bmp` one captured BMP frame

## Important

This is a snapshot web UI, not a low-latency MJPEG/WebRTC streamer. That is intentional for the first stable API: it favors simplicity and predictable memory use.
