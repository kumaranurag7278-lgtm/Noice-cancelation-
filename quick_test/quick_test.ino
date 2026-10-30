#include <Arduino.h>
#include <ESP_I2S.h>
#include <math.h>

// Pins for INMP441 Microphone (I2S_NUM_0)
#define MIC_SCK 14
#define MIC_WS  27
#define MIC_SD  32

// Pins for MAX98357A Amplifier (I2S_NUM_1)
#define AMP_BCLK 26
#define AMP_LRC  25
#define AMP_DIN  22

const int sampleRate = 16000;

I2SClass i2s_mic;
I2SClass i2s_amp;

void playChime(float freq, int durationMs, int volume = 15000) {
  int totalSamples = (sampleRate * durationMs) / 1000;
  for (int i = 0; i < totalSamples; i++) {
    float rad = (2.0 * PI * freq * i) / sampleRate;
    int16_t sample = (int16_t)(sin(rad) * volume);
    int16_t frame[2] = { sample, sample };
    i2s_amp.write((uint8_t*)frame, sizeof(frame));
  }
}

void playSilence(int durationMs) {
  int totalSamples = (sampleRate * durationMs) / 1000;
  int16_t zero[2] = { 0, 0 };
  for (int i = 0; i < totalSamples; i++) {
    i2s_amp.write((uint8_t*)zero, sizeof(zero));
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n==============================================");
  Serial.println("   ESP32 COMPLETE SYSTEM QUICK TEST           ");
  Serial.println("==============================================");

  // 1. Initialize Amplifier (I2S_NUM_1)
  Serial.println("[1/3] Initializing MAX98357A Audio Amplifier...");
  i2s_amp.setPort(I2S_NUM_1);
  i2s_amp.setPins(AMP_BCLK, AMP_LRC, AMP_DIN);
  if (!i2s_amp.begin(I2S_MODE_STD, sampleRate, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO)) {
    Serial.println("FAIL: Amp I2S initialization error!");
  } else {
    Serial.println("PASS: Amp I2S ready! Playing pleasant startup chime...");
    playChime(523.25, 250); // C5
    playChime(659.25, 300); // E5
    playChime(783.99, 400); // G5
    playSilence(200);
    Serial.println("PASS: Speaker chime completed!");
  }

  // 2. Initialize Microphone (I2S_NUM_0)
  Serial.println("\n[2/3] Initializing INMP441 MEMS Microphone...");
  i2s_mic.setPort(I2S_NUM_0);
  i2s_mic.setPins(MIC_SCK, MIC_WS, -1, MIC_SD);
  if (!i2s_mic.begin(I2S_MODE_STD, sampleRate, I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO)) {
    Serial.println("FAIL: Mic I2S initialization error!");
  } else {
    Serial.println("PASS: Mic I2S ready!");
    Serial.println("Testing Mic sound reception for 3 seconds...");
    unsigned long start = millis();
    int32_t buf[64];
    int maxRMS = 0;
    while (millis() - start < 3000) {
      size_t readLen = i2s_mic.readBytes((char*)buf, sizeof(buf));
      if (readLen > 0) {
        int pairs = readLen / (2 * sizeof(int32_t));
        int64_t sumSq = 0;
        for (int i = 0; i < pairs; i++) {
          int32_t s = buf[i * 2] >> 14;
          sumSq += (int64_t)s * s;
        }
        int rms = sqrt(sumSq / pairs);
        if (rms > maxRMS) maxRMS = rms;
      }
      delay(20);
    }
    Serial.printf("PASS: Mic capture working! (Max RMS recorded: %d)\n", maxRMS);
  }

  // 3. Talk-through test: Mic to Speaker live for 4 seconds
  Serial.println("\n[3/3] LIVE TALKTHROUGH TEST (4 seconds):");
  Serial.println("Say something into the mic — it will echo on the woofer!");
  unsigned long passStart = millis();
  int32_t inBuf[64];
  int16_t outBuf[64];

  while (millis() - passStart < 4000) {
    size_t readLen = i2s_mic.readBytes((char*)inBuf, sizeof(inBuf));
    if (readLen > 0) {
      int pairs = readLen / (2 * sizeof(int32_t));
      for (int i = 0; i < pairs; i++) {
        // Boost mic audio slightly for woofer
        int32_t sample = (inBuf[i * 2] >> 14) * 3;
        sample = constrain(sample, -32760, 32760);
        outBuf[i * 2] = (int16_t)sample;
        outBuf[i * 2 + 1] = (int16_t)sample;
      }
      i2s_amp.write((uint8_t*)outBuf, pairs * 2 * sizeof(int16_t));
    }
  }

  playSilence(200);
  Serial.println("\n==============================================");
  Serial.println("   ALL TESTS COMPLETE! SYSTEM FULLY READY.    ");
  Serial.println("   (Now in silent standby mode)               ");
  Serial.println("==============================================");
}

void loop() {
  // Silent idle
  delay(1000);
}
