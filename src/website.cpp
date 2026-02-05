#include "driver/i2s.h"
#include "images.h"
#include "ui.h"
#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>
#include <cmath>
#include <cstring>

WebServer server(80);

const int dacPin = 26;
const int micPin = 33;
float frequency = 440.0;
float volume = 0.1;
// I2S Configuration
#define I2S_NUM I2S_NUM_0
#define SAMPLE_RATE 44100
#define I2S_DMA_BUF_COUNT 8
#define I2S_DMA_BUF_LEN 64

bool isPlaying = false;
bool micControlEnabled = false;
int micValue = 0;

void audioTask(void *pvParameters) {
  size_t bytesWritten;
  uint16_t buffer[I2S_DMA_BUF_LEN];
  float localPhase = 0;
  bool i2sRunning =
      true; // Driver installed in setup() starts running by default

  while (true) {
    if (isPlaying) {
      if (!i2sRunning) {
        i2s_start(I2S_NUM);
        i2sRunning = true;
      }

      float phaseIncrement = 2.0 * PI * frequency / SAMPLE_RATE;

      for (int i = 0; i < I2S_DMA_BUF_LEN; i++) {
        localPhase += phaseIncrement;
        if (localPhase >= 2.0 * PI)
          localPhase -= 2.0 * PI;

        int val = 127 + (int)(volume * 127.0 * sin(localPhase));
        buffer[i] = val << 8;
      }

      i2s_write(I2S_NUM, buffer, sizeof(buffer), &bytesWritten, portMAX_DELAY);
    } else {
      if (i2sRunning) {
        // Write one buffer of silence to settle DAC before stopping
        for (int i = 0; i < I2S_DMA_BUF_LEN; i++)
          buffer[i] = 127 << 8;

        i2s_write(I2S_NUM, buffer, sizeof(buffer), &bytesWritten,
                  portMAX_DELAY);

        i2s_stop(I2S_NUM);
        i2sRunning = false;
      }
      vTaskDelay(50 / portTICK_PERIOD_MS);
    }
  }
}

void handleSet() {
  if (server.hasArg("frequency")) {
    frequency = server.arg("frequency").toFloat();
  }
  if (server.hasArg("volume")) {
    volume = server.arg("volume").toFloat();
  }
  if (server.hasArg("playback")) {
    isPlaying = (server.arg("playback") == "1");
  }
  if (server.hasArg("micEnabled")) {
    micControlEnabled = (server.arg("micEnabled") == "1");
  }

  Serial.print("Update Received -> ");
  Serial.print("Status: ");
  Serial.print(isPlaying ? "RUNNING" : "STOPPED");
  Serial.print(" | Freq: ");
  Serial.print(frequency);
  Serial.print("Hz | Vol: ");
  Serial.print(volume * 100);
  Serial.println("%");

  server.send(200, "text/plain", "OK");
}

void handleStatus() {
  String json = "{";
  json += "\"frequency\":" + String(frequency) + ",";
  json += "\"volume\":" + String(volume) + ",";
  json +=
      "\"micEnabled\":" + String(micControlEnabled ? "true" : "false") + ",";
  json += "\"micValue\":" + String(micValue);
  json += "}";
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  WiFi.softAP("ESP32-Tone-Gen", "12345678");

  server.on("/", []() { server.send(200, "text/html", INDEX_HTML); });
  server.on("/style.css", []() { server.send(200, "text/css", STYLE_CSS); });
  server.on("/script.js",
            []() { server.send(200, "application/javascript", SCRIPT_JS); });

  server.on("/img/low.jpg", []() {
    server.send_P(200, "image/jpeg", (const char *)img_low_jpg,
                  img_low_jpg_len);
  });
  server.on("/img/mid.jpg", []() {
    server.send_P(200, "image/jpeg", (const char *)img_mid_jpg,
                  img_mid_jpg_len);
  });
  server.on("/img/high.jpg", []() {
    server.send_P(200, "image/jpeg", (const char *)img_high_jpg,
                  img_high_jpg_len);
  });

  server.on("/set", handleSet);
  server.on("/status", handleStatus);

  server.begin();

  // I2S Setup
  i2s_config_t i2s_config = {
      .mode =
          (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN),
      .sample_rate = SAMPLE_RATE,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
      .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
      .communication_format = I2S_COMM_FORMAT_STAND_MSB,
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
      .dma_buf_count = I2S_DMA_BUF_COUNT,
      .dma_buf_len = I2S_DMA_BUF_LEN,
      .use_apll = false,
      .tx_desc_auto_clear = true,
      .fixed_mclk = 0};

  i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM, NULL); // INTERNAL_DAC Mode
  i2s_set_dac_mode(
      I2S_DAC_CHANNEL_BOTH_EN); // I2S_DAC_CHANNEL_RIGHT_EN (GPIO26)

  xTaskCreatePinnedToCore(audioTask, "AudioTask", 4096, NULL, 1, NULL,
                          1 // Pin to Core 1 (App Core)
  );

  Serial.println("\n===========================");
  Serial.println("Tone Generator Initialized");
  Serial.print("Access URL: http://");
  Serial.println(WiFi.softAPIP());
  Serial.println("===========================");
  Serial.println("===========================");
}

float mapMicToFreq(float amplitude) {
  float targetFreq = 100.0 + (amplitude * 2.0);
  if (targetFreq > 2000)
    targetFreq = 2000;
  if (targetFreq < 100)
    targetFreq = 100;
  return targetFreq;
}

const int sampleWindow = 50;
unsigned int signalMax = 0;
unsigned int signalMin = 4096;
unsigned long startMillis = 0;

void processMicrophone() {
  if (millis() - startMillis >= sampleWindow) {
    int peakToPeak = signalMax - signalMin;

    const int noiseThreshold = 100;
    if (peakToPeak < noiseThreshold) {
      peakToPeak = 0;
    }

    static float smoothedAmp = 0;
    smoothedAmp = (smoothedAmp * 0.7) + (peakToPeak * 0.3);
    micValue = (int)smoothedAmp;
    if (micControlEnabled) {
      float targetFreq = mapMicToFreq(smoothedAmp);
      frequency = frequency * 0.9 + targetFreq * 0.1;
    }
    Serial.print("Min:");
    Serial.print(signalMin);
    Serial.print(" Max:");
    Serial.print(signalMax);
    Serial.print(" Amp:");
    Serial.println(micValue);

    signalMax = 0;
    signalMin = 4096;
    startMillis = millis();
  }

  int raw = analogRead(micPin);

  if (raw < 4096) {
    if (raw > signalMax) {
      signalMax = raw;
    }
    if (raw < signalMin) {
      signalMin = raw;
    }
  }
}

void loop() {
  server.handleClient();
  processMicrophone();
}