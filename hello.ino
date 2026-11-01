#include <Arduino.h>
#include <driver/i2s.h>
#include "audio_data.h" // Automatically loads your sound.wav array

// I2S Pin Definitions (Match your wiring)
#define I2S_BCLK  26
#define I2S_LRC   25
#define I2S_DOUT  22
#define I2S_NUM   I2S_NUM_0

void setupI2S() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = AUDIO_SAMPLE_RATE, // Automatically uses your WAV file's sample rate
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

void playCustomAudio() {
  size_t bytes_written;
  int16_t buffer[64];

  Serial.println("Playing audio from flash memory...");

  // Stream audio samples from Flash (PROGMEM) to the amplifier
  for (int i = 0; i < AUDIO_LENGTH; i += 32) {
    int chunk = min(32, AUDIO_LENGTH - i);
    
    for (int j = 0; j < chunk; j++) {
      // Read 16-bit sample from flash storage
      int16_t sample = (int16_t)pgm_read_word(&custom_audio[i + j]);
      
      buffer[j * 2]     = sample; // Left channel
      buffer[j * 2 + 1] = sample; // Right channel
    }
    
    i2s_write(I2S_NUM, buffer, chunk * 4, &bytes_written, portMAX_DELAY);
  }
  Serial.println("Playback complete!");
}

void setup() {
  Serial.begin(115200);
  setupI2S();
  
  delay(1500); // Short delay to let things initialize
  playCustomAudio(); // Plays the sound once on startup
}

void loop() {
  // Repeats playback every 5 seconds. 
  // If you only want it to play once on boot, you can leave loop() completely empty!
  delay(5000);
  playCustomAudio();
}
