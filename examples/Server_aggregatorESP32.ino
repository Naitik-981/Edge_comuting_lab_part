// AGGREGATOR: ESP32 Aggregator - Receives from clients, aggregates, sends to HOST

#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

const char* ssid = "YourWiFiSSID";
const char* password = "YourWiFiPassword";
const char* host_url = "http://192.168.1.200/aggregate"; // Edge server (host) IP

WebServer server(80);

struct ClientData {
  String name;
  float temp;
  float hum;
  bool received;
};

ClientData clients[2];

unsigned long lastSend = 0;

void handleSensor() {
  if (server.hasArg("plain")) {
    String body = server.arg("plain");
    StaticJsonDocument<200> doc;
    deserializeJson(doc, body);
    String cname = doc["client_name"] | "";
    float t = doc["temperature"];
    float h = doc["humidity"];
    for (int i = 0; i < 2; i++) {
      if (clients[i].name == "" || clients[i].name == cname) {
        clients[i].name = cname;
        clients[i].temp = t;
        clients[i].hum = h;
        clients[i].received = true;
        break;
      }
    }
    server.send(200, "text/plain", "OK");
    Serial.printf("Received from %s: T=%.2f, H=%.2f\n", cname.c_str(), t, h);
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
  if (millis() - lastSend > 10000) { // Send every 10s
    int count = 0;
    float temp_sum = 0, hum_sum = 0;
    String info = "";
    StaticJsonDocument<256> doc;
    JsonArray arr = doc.createNestedArray("clients");
    for (int i = 0; i < 2; i++) {
      if (clients[i].received) {
        temp_sum += clients[i].temp;
        hum_sum += clients[i].hum;
        count++;
        JsonObject obj = arr.createNestedObject();
        obj["client_name"] = clients[i].name;
        obj["temperature"] = clients[i].temp;
        obj["humidity"] = clients[i].hum;
        clients[i].received = false;
      }
    }
    if (count > 0) {
      doc["temperature_avg"] = temp_sum / count;
      doc["humidity_avg"] = hum_sum / count;
      String payload;
      serializeJson(doc, payload);
      HTTPClient http;
      http.begin(host_url);
      http.addHeader("Content-Type", "application/json");
      http.POST(payload);
      http.end();
      Serial.println("Sent to host: " + payload);
    }
    lastSend = millis();
  }
}
