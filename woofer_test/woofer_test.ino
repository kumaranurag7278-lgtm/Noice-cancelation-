#include <Arduino.h>
#include <ESP_I2S.h>
#include <math.h>

// MAX98357A I2S Amplifier Pinout
#define I2S_AMP_BCLK 26
#define I2S_AMP_LRC  25
#define I2S_AMP_DIN  22

const int sampleRate = 16000;
I2SClass i2s_amp;

void playTone(float freq, int durationMs, int amplitude = 18000) {
  int totalSamples = (sampleRate * durationMs) / 1000;
  for (int i = 0; i < totalSamples; i++) {
    float rad = (2.0 * PI * freq * i) / sampleRate;
    int16_t val = (int16_t)(sin(rad) * amplitude);
    int16_t frame[2] = { val, val };
    i2s_amp.write((uint8_t*)frame, sizeof(frame));
  }
}

void playSweep(float startFreq, float endFreq, int durationMs, int amplitude = 18000) {
  int totalSamples = (sampleRate * durationMs) / 1000;
  float phase = 0.0;
  for (int i = 0; i < totalSamples; i++) {
    float progress = (float)i / totalSamples;
    float currentFreq = startFreq + (endFreq - startFreq) * progress;
    phase += (2.0 * PI * currentFreq) / sampleRate;
    if (phase > 2.0 * PI) phase -= 2.0 * PI;
    int16_t val = (int16_t)(sin(phase) * amplitude);
    int16_t frame[2] = { val, val };
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
  Serial.println("\n========================================");
  Serial.println("  DY VOICE 4-INCH WOOFER & AMP TEST     ");
  Serial.println("========================================");

  i2s_amp.setPort(I2S_NUM_0);
  i2s_amp.setPins(I2S_AMP_BCLK, I2S_AMP_LRC, I2S_AMP_DIN);

  if (!i2s_amp.begin(I2S_MODE_STD, sampleRate, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO)) {
    Serial.println("ERROR: Failed to initialize I2S for Amplifier!");
    while (1) delay(100);
  }

  Serial.println("Amplifier I2S ready!");
  Serial.println("Starting Woofer Bass & Audio Sequence...");
}

void loop() {
  Serial.println("\n[1] Low Bass Test (120 Hz) - Feel the Woofer Cone Vibrate!");
  playTone(120, 1200, 22000); // 120 Hz deep bass
  playSilence(200);

  Serial.println("[2] Mid Punch Test (350 Hz)...");
  playTone(350, 800, 20000);
  playSilence(200);

  Serial.println("[3] High Tone Test (800 Hz)...");
  playTone(800, 800, 18000);
  playSilence(200);

  Serial.println("[4] Frequency Sweep (80 Hz -> 1000 Hz)...");
  playSweep(80, 1000, 2000, 20000);
  playSilence(500);

  Serial.println("[5] Double Alert Beep...");
  playTone(600, 250, 20000);
  playSilence(100);
  playTone(900, 350, 20000);
  playSilence(1500);
}
