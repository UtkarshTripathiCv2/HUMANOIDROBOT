#include <Arduino.h>
#include <driver/i2s.h>

// I2S Pin Definitions
#define I2S_BCLK  26
#define I2S_LRC   25
#define I2S_DOUT  22
#define I2S_NUM   I2S_NUM_0

const int SAMPLE_RATE = 16000;

// Musical Note Frequencies (Hz)
#define NOTE_C4  261.63
#define NOTE_D4  293.66
#define NOTE_E4  329.63
#define NOTE_F4  349.23
#define NOTE_G4  392.00
#define NOTE_A4  440.00
#define NOTE_B4  493.88
#define NOTE_C5  523.25

void setupI2S() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 64,
    .use_apll = false,
    .tx_desc_auto_clear = true
  };

  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_BCLK,
    .ws_io_num = I2S_LRC,
    .data_out_num = I2S_DOUT,
    .data_in_num = I2S_PIN_NO_CHANGE
  };

  i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM, &pin_config);
}

// Function to play an individual musical note
void playNote(float freq, int duration_ms, int volume = 6000) {
  int num_samples = (SAMPLE_RATE * duration_ms) / 1000;
  float angle_step = 2.0 * PI * freq / SAMPLE_RATE;
  float current_angle = 0;
  
  size_t bytes_written;
  int16_t buffer[64];

  int samples_sent = 0;
  while (samples_sent < num_samples) {
    int chunk = min(32, num_samples - samples_sent);
    for (int i = 0; i < chunk; i++) {
      int16_t sample = (int16_t)(sin(current_angle) * volume);
      buffer[i * 2]     = sample; // Left channel
      buffer[i * 2 + 1] = sample; // Right channel
      current_angle += angle_step;
      if (current_angle >= 2.0 * PI) current_angle -= 2.0 * PI;
    }
    i2s_write(I2S_NUM, buffer, chunk * 4, &bytes_written, portMAX_DELAY);
    samples_sent += chunk;
  }
}

// Function to add a short pause between notes
void playRest(int duration_ms) {
  int num_samples = (SAMPLE_RATE * duration_ms) / 1000;
  size_t bytes_written;
  int16_t silence[64] = {0};
  
  int samples_sent = 0;
  while (samples_sent < num_samples) {
    int chunk = min(32, num_samples - samples_sent);
    i2s_write(I2S_NUM, silence, chunk * 4, &bytes_written, portMAX_DELAY);
    samples_sent += chunk;
  }
}

void setup() {
  Serial.begin(115200);
  setupI2S();
  
  Serial.println("\nPlaying song melody directly from ESP32 flash memory...");

  // Song Sequence (e.g., cheerful opening jingle)
  playNote(NOTE_C4, 200);
  playNote(NOTE_E4, 200);
  playNote(NOTE_G4, 200);
  playNote(NOTE_C5, 400);
  playRest(100);
  
  playNote(NOTE_G4, 200);
  playNote(NOTE_B4, 200);
  playNote(NOTE_C5, 600);
  
  Serial.println("Playback finished!");
}

void loop() {
  // Play again every 5 seconds if desired, or leave blank to play once on boot
  delay(5000);
  playNote(NOTE_C4, 200);
  playNote(NOTE_E4, 200);
  playNote(NOTE_G4, 200);
  playNote(NOTE_C5, 400);
}
