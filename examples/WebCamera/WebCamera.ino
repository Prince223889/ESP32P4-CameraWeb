#include <ESP32P4Camera.h>
#include <ESP32P4CameraWeb.h>

ESP32P4Camera camera;
ESP32P4CameraWeb web;

void setup() {
  Serial.begin(115200);
  delay(500);

  if (!camera.begin()) {
    Serial.println("CAMERA_INIT=FAILED");
    return;
  }
  // SSID/password can be changed to your own values.
  if (!web.begin(camera, "ESP32P4-Camera", "12345678")) {
    Serial.println("WEB_INIT=FAILED");
    return;
  }
  Serial.println("WEB_INIT=OK");
  Serial.println("Connect your phone to ESP32P4-Camera, then open the IP shown above.");
}

void loop() {
  web.loop();
  delay(2);
}
