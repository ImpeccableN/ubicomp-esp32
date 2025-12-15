#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "ESP32-Control";
const char* password = "12345678";

const int CONTROL_PIN = 26;

WebServer server(80);

String htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<title>ESP32 Pin Control</title>
<style>
body { font-family: Arial; text-align: center; padding-top: 30px; }
button {
  width: 120px; height: 50px;
  font-size: 20px; margin: 10px;
}
.slider {
  -webkit-appearance: none;  /* Override default CSS styles */
  appearance: none;
  width: 100%; /* Full-width */
  height: 25px; /* Specified height */
  background: #d3d3d3; /* Grey background */
  outline: none; /* Remove outline */
  opacity: 0.7; /* Set transparency (for mouse-over effects on hover) */
  -webkit-transition: .2s; /* 0.2 seconds transition on hover */
  transition: opacity .2s;
}
</style>
</head>
<body>
<h2>ESP32 Pin Toggle</h2>
<button onclick="location.href='/on'">ON</button>
<button onclick="location.href='/off'">OFF</button>
<input type="range" min="1440" max="30000" value="1440" class="slider" id="waveRange" oninput="location.href='/waveChange?value=' + this.value">
</body>
</html>
)rawliteral";

#include <Arduino.h>
#include <AudioTools.h>


uint16_t sample_rate = 44100;
uint8_t channels = 2;
SineWaveGenerator<int16_t> sineWave(32000);
GeneratedSoundStream<int16_t> sound(sineWave);
CsvOutput<int16_t> out(Serial);
StreamCopy copier(out, sound);



void handleRoot() {
  server.send(200, "text/html", htmlPage);
}

void handleOn() {
  digitalWrite(CONTROL_PIN, HIGH);
  server.send(200, "text/html", htmlPage);
}

void handleOff() {
  digitalWrite(CONTROL_PIN, LOW);
  server.send(200, "text/html", htmlPage);
}

void handleWaveChange() {

}

void setup() {
  Serial.begin(115200);

  pinMode(CONTROL_PIN, OUTPUT);
  digitalWrite(CONTROL_PIN, LOW);

  Serial.println("Starting Access Point...");

  // Start WiFi Access Point
  WiFi.softAP(ssid, password);

  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP()); // should print 192.168.4.1

  server.on("/", handleRoot);
  server.on("/on", handleOn);
  server.on("/off", handleOff);
  server.on("/waveChange?freq=" + val, handleWaveChange);

  server.begin();
  Serial.println("Web server started!");

    // open Serial
  // Serial.begin(9600);
  // AudioLogger::instance().begin(Serial, AudioLogger::Info);

  // defince CSV output
  auto config = out.defaultConfig();
  config.sample_rate = sample_rate;
  config.channels = channels;
  out.begin(config);

  // Setup sine wave
  sineWave.begin(channels, sample_rate, N_B4);
  Serial.println("started...");
}

void loop() {
  server.handleClient();
    // copy sound to out
  copier.copy();
}
