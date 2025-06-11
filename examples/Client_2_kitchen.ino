// CLIENT: ESP32 Sensor Node - Kitchen

#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>
#define DHTPIN 5
#define DHTTYPE DHT11

const char* ssid = "YourWiFiSSID";
const char* password = "YourWiFiPassword";
const char* aggregator_url = "http://192.168.1.100/sensor"; // Aggregator ESP32 IP

const char* client_name = "Kitchen";

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  dht.begin();
}

void loop() {
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  if (!isnan(temp) && !isnan(hum)) {
    HTTPClient http;
    http.begin(aggregator_url);
    http.addHeader("Content-Type", "application/json");
    String payload = "{\"client_name\":\"" + String(client_name) + "\",\"temperature\":" + String(temp) + ",\"humidity\":" + String(hum) + "}";
    http.POST(payload);
    http.end();
    Serial.println("Sent data: " + payload);
  }
  delay(5000);
}
