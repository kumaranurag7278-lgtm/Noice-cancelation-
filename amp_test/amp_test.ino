#include <Arduino.h>
#include <ESP_I2S.h>
#include <math.h>

#define I2S_BCLK 26
#define I2S_LRC  25
#define I2S_DIN  22

const int sampleRate = 16000;
i2s_data_bit_width_t bps = I2S_DATA_BIT_WIDTH_16BIT;
i2s_mode_t mode = I2S_MODE_STD;
i2s_slot_mode_t slot = I2S_SLOT_MODE_STEREO;

I2SClass i2s;

void playTone(float freq, int durationMs, int volume = 6000) {
  int totalSamples = (sampleRate * durationMs) / 1000;
  for (int i = 0; i < totalSamples; i++) {
    // Generate clean sine wave
    float rad = (2.0 * PI * freq * i) / sampleRate;
    int16_t sample = (int16_t)(sin(rad) * volume);
    int16_t stereo[2] = { sample, sample };
    i2s.write((uint8_t*)stereo, sizeof(stereo));
  }
}

void playSilence(int durationMs) {
  int totalSamples = (sampleRate * durationMs) / 1000;
  int16_t zero[2] = { 0, 0 };
  for (int i = 0; i < totalSamples; i++) {
    i2s.write((uint8_t*)zero, sizeof(zero));
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=================================");
  Serial.println("  ESP32 -> MAX98357 Audio Melody  ");
  Serial.println("=================================");

  i2s.setPins(I2S_BCLK, I2S_LRC, I2S_DIN);
  if (!i2s.begin(mode, sampleRate, bps, slot)) {
    Serial.println("FAILED to initialize I2S!");
    while (1) delay(100);
  }
  Serial.println("I2S Audio ready! Playing Melody Loop...");
}

// Melody frequencies (C5, E5, G5, C6, G5, E5, C5)
const float notes[] = { 523.25, 659.25, 783.99, 1046.50, 783.99, 659.25, 523.25 };
const int numNotes = sizeof(notes) / sizeof(notes[0]);

void loop() {
  for (int i = 0; i < numNotes; i++) {
    Serial.printf("Playing Note %d: %.1f Hz\n", i + 1, notes[i]);
    playTone(notes[i], 200, 8000);  // 200ms note at strong volume
    playSilence(40);
  }
  playSilence(800); // 800ms pause before repeating
}
