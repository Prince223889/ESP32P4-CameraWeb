# ESP32P4-CameraWeb

A lightweight web interface for an ESP32-P4 camera board.

## What it does

1. Starts the P4 MIPI-CSI camera through `ESP32P4-Camera`.
2. Starts the P4's Hosted-Wi-Fi SoftAP (the Waveshare board uses an on-board ESP32-C6 radio coprocessor).
3. Prints the AP name and IP address in Serial Monitor.
4. Serves one small HTML page.
5. Refreshes only the `<img>` URL when a photo is requested; the page itself is not reloaded.
6. Streams the captured frame to the browser as an uncompressed 24-bit BMP.

## Example

```cpp
#include <ESP32P4CameraWeb.h>
ESP32P4CameraWeb cameraWeb;

void setup() {
  Serial.begin(115200);
  cameraWeb.begin("ESP32-P4-Camera", "12345678");
}

void loop() {
  cameraWeb.handleClient();
}
```

## Browser workflow

- Flash the example.
- Open Serial Monitor at 115200 baud.
- Connect to the printed Wi-Fi SSID.
- Open the printed IP address, normally the SoftAP interface address.
- `Take photo` refreshes only the image.
- `Auto` periodically refreshes only the image.
- `Save image` requests a fresh BMP.

## Why BMP instead of JPEG in v1

The first release deliberately avoids a custom JPEG encoder API in the Arduino wrapper. The P4/ESP-Video stack can work with encoded video paths, but a small library should have one simple deterministic capture path first. RGB565 -> BMP is CPU/memory predictable and requires no third-party decoder in the browser.

For large images, BMP is larger than JPEG. This is a documented trade-off, not a hidden limitation.

## Important board notes

On the Waveshare ESP32-P4-WIFI6-DEV-KIT, the camera-control I2C bus is SDA GPIO7 / SCL GPIO8. Do not reuse the ESP32-C6 Hosted-Wi-Fi SDIO pins (GPIO14-19) for your sensor wiring on this board.

The web library depends on `ESP32P4-Camera` and a recent Arduino-ESP32 core with `ESP_Video` available for ESP32-P4.

## Compatibility aliases

`ESP32P4CameraWeb` is the primary class. `P4CameraWeb` is provided as an alias, so the earlier compile error

`'ESP32P4CameraWeb' was not declared in this scope; did you mean 'P4CameraWeb'?`

is avoided as long as the library header is installed correctly.
