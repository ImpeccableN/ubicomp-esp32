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
  server.on("/script.js", []() { server.send(200, "application/javascript", SCRIPT_JS); });
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

void processMicrophone() {
  int raw = analogRead(micPin);

  // (MAX9814 mic/amp has bias of 1.25V. 1.25/3.3 * 4096 ~= 1551)
  const int dcOffset = 1551;
  int amplitude = abs(raw - dcOffset);

  static float smoothedAmp = 0;
  smoothedAmp = (smoothedAmp * 0.9) + (amplitude * 0.1);
  micValue = (int)smoothedAmp;

  if (micControlEnabled) {
    float targetFreq = mapMicToFreq(smoothedAmp);
    frequency = frequency * 0.9 + targetFreq * 0.1;
  }

  static unsigned long lastLog = 0;
  if (millis() - lastLog > 50) {
    Serial.print("Raw:");
    Serial.print(raw);
    Serial.print(",");
    Serial.print("Amp:");
    Serial.println((int)smoothedAmp);
    lastLog = millis();
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
    if (phase > 2.0 * PI) phase -= 2.0 * PI;

    int val = 128 + (int)(volume * 127.0 * sin(phase));
    dacWrite(dacPin, val);
  } else {
    dacWrite(dacPin, 128);
  }
}