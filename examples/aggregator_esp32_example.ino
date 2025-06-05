// Layer 2: Aggregator Node Example (ESP32)
// Receives data from sensor nodes and forwards to edge server

#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <HTTPClient.h>

const char* ssid = "YourWiFiSSID";
const char* password = "YourWiFiPassword";
const char* edge_server_url = "http://192.168.1.200/aggregate";

WebServer server(80);

float tempSum = 0, humSum = 0;
int sampleCount = 0;
unsigned long lastSend = 0;

void handleSensor() {
  if (server.hasArg("plain")) {
    String body = server.arg("plain");
    int tIdx = body.indexOf("temperature");
    int hIdx = body.indexOf("humidity");
    float t = body.substring(tIdx + 13, body.indexOf(",", tIdx)).toFloat();
    float h = body.substring(hIdx + 10, body.indexOf("}", hIdx)).toFloat();
    tempSum += t;
    humSum += h;
    sampleCount++;
    server.send(200, "text/plain", "OK");
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  server.on("/sensor", HTTP_POST, handleSensor);
  server.begin();
}

void loop() {
  server.handleClient();
  if (millis() - lastSend > 30000 && sampleCount > 0) {
    float avgT = tempSum / sampleCount;
    float avgH = humSum / sampleCount;
    HTTPClient http;
    http.begin(edge_server_url);
    http.addHeader("Content-Type", "application/json");
    String payload = "{\"temperature\":" + String(avgT) + ",\"humidity\":" + String(avgH) + "}";
    http.POST(payload);
    http.end();
    tempSum = 0; humSum = 0; sampleCount = 0; lastSend = millis();
  }
}