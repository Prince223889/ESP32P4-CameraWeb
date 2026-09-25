#include <ESP32P4CameraWeb.h>

ESP32P4CameraWeb cameraWeb;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("========================================");
  Serial.println(" ESP32-P4 WEB CAMERA");
  Serial.println("========================================");

  if (!cameraWeb.begin("ESP32-P4-Camera", "12345678")) {
    Serial.println("[ERROR] Camera/Web server start failed.");
    return;
  }

  Serial.println("[OK] Connect your phone/PC to the Wi-Fi network above.");
  Serial.println("[OK] Open the printed IP address in a browser.");
}

void loop() {
  cameraWeb.handleClient();
  delay(2);
}
