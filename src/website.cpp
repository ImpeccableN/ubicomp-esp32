#include "images.h"
#include "ui.h"
#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>

WebServer server(80);

const int dacPin = 26;
const int micPin = 33;
float frequency = 440.0;
float volume = 0.1;
float phase = 0;
bool isPlaying = false;
bool micControlEnabled = false;
int micValue = 0;

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

  if (isPlaying) {
    static unsigned long lastMicros = 0;
    unsigned long currentMicros = micros();
    float deltaTime = (currentMicros - lastMicros) / 1000000.0;
    lastMicros = currentMicros;

    phase += 2.0 * PI * frequency * deltaTime;
    if (phase > 2.0 * PI)
      phase -= 2.0 * PI;

    int val = 128 + (int)(volume * 127.0 * sin(phase));
    dacWrite(dacPin, val);
  } else {
    dacWrite(dacPin, 128);
  }
}