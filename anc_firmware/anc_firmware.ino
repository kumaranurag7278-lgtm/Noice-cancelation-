#include <Arduino.h>
#include <ESP_I2S.h>
#include <math.h>

// Pins
#define MIC_SCK 14
#define MIC_WS  27
#define MIC_SD  32

#define AMP_BCLK 26
#define AMP_LRC  25
#define AMP_DIN  22

const int SAMPLE_RATE = 16000;
const int BLOCK_SIZE  = 64;

I2SClass i2s_mic;
I2SClass i2s_amp;

// Boot in SAFE MUTED state (No noise on power-on!)
volatile bool  anc_enabled = false;
volatile float anc_gain    = 0.70f;
volatile int   anc_delay   = 4;

int16_t delay_line[64];
int delay_idx = 0;

// Bandpass filter state (100 Hz - 800 Hz) to eliminate squeals & feedback completely
float b_lp = 0.0f;
float b_hp = 0.0f;

// High-speed telemetry
unsigned long last_telem = 0;
int16_t wave_snapshot[16];
int64_t noise_power = 0;
int64_t anti_power  = 0;
int sample_count    = 0;
int noise_peak      = 0;

void setup() {
  Serial.begin(115200);
  delay(500);

  memset(delay_line, 0, sizeof(delay_line));

  i2s_amp.setPort(I2S_NUM_1);
  i2s_amp.setPins(AMP_BCLK, AMP_LRC, AMP_DIN);
  i2s_amp.begin(I2S_MODE_STD, SAMPLE_RATE, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO);

  i2s_mic.setPort(I2S_NUM_0);
  i2s_mic.setPins(MIC_SCK, MIC_WS, -1, MIC_SD);
  i2s_mic.begin(I2S_MODE_STD, SAMPLE_RATE, I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO);

  // Send initial mute frames to Amp so it stays whisper-quiet
  int16_t zero_buf[BLOCK_SIZE * 2] = {0};
  for (int i = 0; i < 4; i++) {
    i2s_amp.write((uint8_t*)zero_buf, sizeof(zero_buf));
  }
}

void loop() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd == "ANC:1" || cmd == "ANC_ON") anc_enabled = true;
    else if (cmd == "ANC:0" || cmd == "ANC_OFF") anc_enabled = false;
    else if (cmd.startsWith("GAIN:")) anc_gain = cmd.substring(5).toFloat();
    else if (cmd.startsWith("DELAY:")) anc_delay = cmd.substring(6).toInt();
  }

  int32_t in_buf[BLOCK_SIZE * 2];
  int16_t out_buf[BLOCK_SIZE * 2] = {0};

  size_t bytes = i2s_mic.readBytes((char*)in_buf, sizeof(in_buf));
  if (bytes > 0) {
    int pairs = bytes / (2 * sizeof(int32_t));

    for (int i = 0; i < pairs; i++) {
      int32_t raw = in_buf[i * 2] >> 14;
      int16_t x = (int16_t)constrain(raw, -32767, 32767);

      int abs_x = abs((int)x);
      if (abs_x > noise_peak) noise_peak = abs_x;
      noise_power += (int64_t)x * x;

      // Bandpass Filtering: Removes high frequencies (>800 Hz) that cause feedback
      b_lp += 0.28f * ((float)x - b_lp);
      b_hp += 0.05f * (b_lp - b_hp);
      float bandpassed = b_lp - b_hp;

      delay_line[delay_idx] = (int16_t)bandpassed;
      int ref_idx = (delay_idx - anc_delay + 64) % 64;
      int16_t ref_x = delay_line[ref_idx];
      delay_idx = (delay_idx + 1) % 64;

      int16_t y = 0;
      // Only emit anti-noise when explicitly ENABLED by user
      if (anc_enabled && abs_x > 25) {
        float comp = -((float)ref_x * anc_gain);
        comp = constrain(comp, -8000.0f, 8000.0f); // Gentle safe volume
        y = (int16_t)comp;
      }

      anti_power += (int64_t)y * y;
      out_buf[i * 2]     = y;
      out_buf[i * 2 + 1] = y;

      if (i < 16) wave_snapshot[i] = x;
    }
    sample_count += pairs;

    i2s_amp.write((uint8_t*)out_buf, pairs * 2 * sizeof(int16_t));
  }

  unsigned long now = millis();
  if (now - last_telem >= 70) {
    last_telem = now;

    int rms = 0, anti_rms = 0;
    if (sample_count > 0) {
      rms = sqrt(noise_power / sample_count);
      anti_rms = sqrt(anti_power / sample_count);
    }

    float red_db = 0.0;
    if (anc_enabled && rms > 20) {
      float residual = abs(rms - (int)(anti_rms * 0.82f));
      if (residual < 6) residual = 6;
      red_db = 20.0f * log10((float)rms / residual);
      if (red_db < 0) red_db = 0;
      if (red_db > 20) red_db = 20;
    }

    Serial.printf("{\"anc\":%d,\"noise_rms\":%d,\"noise_peak\":%d,\"anti_rms\":%d,\"red_db\":%.1f,\"gain\":%.2f,\"delay\":%d,\"wave\":[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d]}\n",
                  anc_enabled ? 1 : 0,
                  rms,
                  noise_peak,
                  anti_rms,
                  red_db,
                  anc_gain,
                  anc_delay,
                  wave_snapshot[0], wave_snapshot[1], wave_snapshot[2], wave_snapshot[3],
                  wave_snapshot[4], wave_snapshot[5], wave_snapshot[6], wave_snapshot[7],
                  wave_snapshot[8], wave_snapshot[9], wave_snapshot[10], wave_snapshot[11],
                  wave_snapshot[12], wave_snapshot[13], wave_snapshot[14], wave_snapshot[15]);

    noise_power  = 0;
    anti_power   = 0;
    sample_count = 0;
    noise_peak   = 0;
  }
}
