#include <Arduino.h>
#include <ESP_I2S.h>
#include <math.h>

#define I2S_MIC_SCK 14
#define I2S_MIC_WS  27
#define I2S_MIC_SD  32

I2SClass i2s;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n--- INMP441 Calibration Test ---");

  i2s.setPins(I2S_MIC_SCK, I2S_MIC_WS, -1, I2S_MIC_SD);

  // Read Stereo 32-bit so Left (Mic) and Right (empty) channels are clearly separated
  if (!i2s.begin(I2S_MODE_STD, 16000, I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO)) {
    Serial.println("Failed to init I2S!");
    while(1) delay(100);
  }
  Serial.println("I2S Stereo Ready. Reading Left channel audio...");
}

void loop() {
  // Read a block of stereo frames: [Left, Right, Left, Right...]
  int32_t buffer[64]; // 32 stereo pairs
  size_t bytesRead = i2s.readBytes((char*)buffer, sizeof(buffer));

  if (bytesRead > 0) {
    int pairs = bytesRead / (2 * sizeof(int32_t));
    int64_t sumSq = 0;
    int32_t peak = 0;

    for (int i = 0; i < pairs; i++) {
      // Left channel is at even index (i * 2)
      int32_t rawLeft = buffer[i * 2];
      // INMP441 puts 24-bit audio MSB-aligned in 32-bit word -> shift down 14 bits for 16-bit range
      int32_t sample = rawLeft >> 14;
      if (abs(sample) > peak) peak = abs(sample);
      sumSq += (int64_t)sample * sample;
    }

    int rms = sqrt(sumSq / pairs);

    // Print Bar
    Serial.printf("RMS:%5d | Peak:%5d | ", rms, peak);
    int bars = map(constrain(rms, 0, 4000), 0, 4000, 0, 35);
    for (int b = 0; b < bars; b++) Serial.print("=");
    Serial.println();
    delay(50);
  }
}
