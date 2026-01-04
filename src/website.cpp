#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "ui.h"

WebServer server(80);

const int dacPin = 26;
float frequency = 440.0;
float volume = 0.1;
float phase = 0;
bool isPlaying = false;

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
    
    Serial.print("Update Received -> ");
    Serial.print("Status: "); Serial.print(isPlaying ? "RUNNING" : "STOPPED");
    Serial.print(" | Freq: "); Serial.print(frequency);
    Serial.print("Hz | Vol: "); Serial.print(volume * 100);
    Serial.println("%");
                  
    server.send(200, "text/plain", "OK");
}

void setup() {
    Serial.begin(115200);
    WiFi.softAP("ESP32-Tone-Gen", "12345678");

    server.on("/", []() { server.send(200, "text/html", INDEX_HTML); });
    server.on("/style.css", []() { server.send(200, "text/css", STYLE_CSS); });
    server.on("/script.js", []() { server.send(200, "application/javascript", SCRIPT_JS); });
    server.on("/set", handleSet);

    server.begin();
    Serial.println("\n===========================");
    Serial.println("Tone Generator Initialized");
    Serial.print("Access URL: http://");
    Serial.println(WiFi.softAPIP());
    Serial.println("===========================");
}

void loop() {
    server.handleClient();

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