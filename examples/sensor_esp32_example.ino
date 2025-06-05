// Layer 1: Sensor Node Example (ESP32)
// Replace with your own WiFi credentials and aggregator IP

#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>
#define DHTPIN 4
#define DHTTYPE DHT11

const char* ssid = "YourWiFiSSID";
const char* password = "YourWiFiPassword";
const char* aggregator_url = "http://192.168.1.100/sensor";

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
    String payload = "{\"temperature\":" + String(temp) + ",\"humidity\":" + String(hum) + "}";
    http.POST(payload);
    http.end();
  }
  delay(5000);
}