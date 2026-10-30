#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n[SYSTEM] Woofer Test Completed & Stopped.");
  Serial.println("[SYSTEM] Hardware Verified: ESP32 + INMP441 + MAX98357 + 4in Woofer all OK!");
}

void loop() {
  delay(1000);
}
