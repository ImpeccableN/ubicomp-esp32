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
</style>
</head>
<body>
<h2>ESP32 Pin Toggle</h2>
<button onclick="location.href='/on'">ON</button>
<button onclick="location.href='/off'">OFF</button>
</body>
</html>
)rawliteral";

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

  server.begin();
  Serial.println("Web server started!");
}

void loop() {
  server.handleClient();
}
