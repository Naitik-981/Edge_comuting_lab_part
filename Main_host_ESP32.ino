// HOST: Arduino UNO R4 WiFi - Receives/Displays Data from Aggregator (Modern Dashboard, shows per-room data)

#include <WiFiS3.h>
#include <WiFiServer.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>

const char* ssid = "YourWiFiSSID";
const char* password = "YourWiFiPassword";

WiFiServer server(80);

#define MAX_SAMPLES 30
float temperature[MAX_SAMPLES];
float humidity[MAX_SAMPLES];
unsigned long timestamps[MAX_SAMPLES];
int sample_count = 0;

// For showing per-client latest data
struct ClientData {
  String name;
  float temp;
  float hum;
};
ClientData clients[2];

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.print("\nHost Arduino R4 WiFi IP: "); Serial.println(WiFi.localIP());
  server.begin();
}

String readPostBody(WiFiClient& client) {
  String body = "";
  bool isBody = false;
  int contentLength = 0;
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line.startsWith("Content-Length:")) {
      contentLength = line.substring(15).toInt();
    }
    if (line == "\r") {
      isBody = true;
      break;
    }
  }
  if (isBody && contentLength > 0) {
    while (client.available() < contentLength) delay(1);
    for (int i = 0; i < contentLength; i++) {
      char c = client.read();
      body += c;
    }
  }
  return body;
}

void loop() {
  WiFiClient client = server.available();
  if (client) {
    while (client.connected() && !client.available()) delay(1);
    String reqLine = client.readStringUntil('\n');
    reqLine.trim();

    if (reqLine.startsWith("POST /aggregate")) {
      String postBody = readPostBody(client);
      Serial.print("Received JSON: ");
      Serial.println(postBody);

      // Parse JSON and store
      StaticJsonDocument<512> doc;
      DeserializationError error = deserializeJson(doc, postBody);
      if (!error) {
        float temp = doc["temperature_avg"] | NAN;
        float hum = doc["humidity_avg"] | NAN;
        if (!isnan(temp) && !isnan(hum)) {
          temperature[sample_count % MAX_SAMPLES] = temp;
          humidity[sample_count % MAX_SAMPLES] = hum;
          timestamps[sample_count % MAX_SAMPLES] = millis() / 1000;
          sample_count++;
        }

        JsonArray arr = doc["clients"];
        int i = 0;
        for (JsonObject obj : arr) {
          if (i < 2) {
            clients[i].name = obj["client_name"].as<String>();
            clients[i].temp = obj["temperature"];
            clients[i].hum = obj["humidity"];
          }
          i++;
        }
      }
      // Respond OK
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/plain");
      client.println("Connection: close");
      client.println();
      client.println("OK");
    } else if (reqLine.startsWith("GET /data")) {
      // Serve JSON data for graph and meters
      StaticJsonDocument<768> doc;
      JsonArray tArr = doc.createNestedArray("temperature");
      JsonArray hArr = doc.createNestedArray("humidity");
      JsonArray tsArr = doc.createNestedArray("timestamps");
      int n = (sample_count > MAX_SAMPLES) ? MAX_SAMPLES : sample_count;
      for (int i = sample_count - n; i < sample_count; i++) {
        int idx = (i + MAX_SAMPLES) % MAX_SAMPLES;
        tArr.add(temperature[idx]);
        hArr.add(humidity[idx]);
        tsArr.add(timestamps[idx]);
      }
      // Add current temp/hum for meters
      doc["currentTemperature"] = (n>0) ? temperature[(sample_count-1)%MAX_SAMPLES] : 0;
      doc["currentHumidity"] = (n>0) ? humidity[(sample_count-1)%MAX_SAMPLES] : 0;

      // Per-client info
      JsonArray clientArr = doc.createNestedArray("clients");
      for (int i = 0; i < 2; i++) {
        JsonObject obj = clientArr.createNestedObject();
        obj["client_name"] = clients[i].name;
        obj["temperature"] = clients[i].temp;
        obj["humidity"] = clients[i].hum;
      }

      String out;
      serializeJson(doc, out);
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: application/json");
      client.println("Connection: close");
      client.println();
      client.println(out);
    } else if (reqLine.startsWith("GET /") || reqLine.startsWith("GET / ")) {
      // Serve Modern Dashboard Webpage (shows per-room data)
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/html");
      client.println("Connection: close");
      client.println();
      client.println(R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Room Climate Dashboard</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
  <script src="https://cdn.jsdelivr.net/npm/gaugeJS/dist/gauge.min.js"></script>
  <style>
    html, body {
      height: 100%;
      margin: 0;
      padding: 0;
      background: linear-gradient(135deg, #e3f0ff 0%, #fae8ff 100%);
      font-family: 'Segoe UI', Arial, sans-serif;
      box-sizing: border-box;
    }
    body {
      min-height: 100vh;
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: flex-start;
      overflow-x: hidden;
    }
    h2 {
      color: #253858;
      margin-top: 24px;
      margin-bottom: 10px;
      font-size: 2em;
      font-weight: 700;
      letter-spacing: 2px;
      text-shadow: 0 2px 8px #d7e9ff5e;
    }
    .dashboard-area {
      width: 97vw;
      max-width: 1150px;
      box-sizing: border-box;
      padding: 0 0 40px 0;
      display: flex;
      flex-direction: column;
      align-items: stretch;
    }
    .card-row {
      display: flex;
      gap: 36px;
      justify-content: center;
      margin-bottom: 22px;
      flex-wrap: wrap;
      width: 100%;
    }
    .card {
      background: rgba(255,255,255,0.97);
      border-radius: 24px;
      box-shadow: 0 6px 32px 0 #7fb3ff33, 0 2px 4px #a98eef0d;
      padding: 28px 32px 18px 32px;
      min-width: 240px;
      width: 350px;
      display: flex;
      flex-direction: column;
      align-items: center;
      transition: box-shadow 0.2s;
    }
    .card:hover {
      box-shadow: 0 12px 48px 0 #7fb3ff4a, 0 4px 8px #a98eef17;
    }
    .gauge-label {
      font-size: 1.13em;
      font-weight: 600;
      margin-bottom: 2px;
      color: #253858;
      transition: color 0.2s;
    }
    .gauge-value {
      font-size: 2.1em;
      font-weight: bold;
      margin-top: 8px;
      margin-bottom: 0;
      letter-spacing: 1.5px;
      text-shadow: 0 1px 6px #aeeaff32;
    }
    #tempVal { color: #f15454; }
    #humVal { color: #3584e4; }
    canvas {
      max-width: 100%;
      max-height: 140px;
      width: 140px !important;
      height: 140px !important;
      margin-bottom: 8px;
      margin-top: 4px;
      border-radius: 12px;
      background: #f7fbff;
    }
    .chart-canvas {
      width: 100% !important;
      height: 120px !important;
      max-width: 340px;
      max-height: 120px;
      margin-bottom: 0;
      margin-top: 10px;
      background: #f9f7ff;
      border-radius: 10px;
      box-shadow: 0 1px 4px #f4e1ff22;
    }
    .chart-label {
      font-size: 1.08em;
      font-weight: 500;
      color: #8e44ad;
      margin-bottom: -4px;
      margin-top: 6px;
      letter-spacing: 0.6px;
    }
    .per-room-data {
      width: 100%;
      display: flex;
      justify-content: center;
      gap: 24px;
      margin-top: 16px;
      margin-bottom: 6px;
      flex-wrap: wrap;
    }
    .per-room-card {
      background: #f7fbff;
      border-radius: 16px;
      box-shadow: 0 2px 8px #d7e9ff33;
      padding: 10px 26px 8px 26px;
      min-width: 140px;
      max-width: 260px;
      text-align: center;
    }
    .per-room-title {
      font-weight: 700;
      color: #8e44ad;
      margin-bottom: 0;
      margin-top: 2px;
      font-size: 1em;
    }
    .per-room-value {
      font-size: 1.25em;
      font-weight: 600;
      margin-bottom: 0;
    }
    .per-room-temp { color: #e04a4a; }
    .per-room-hum { color: #3a6bad; }
    @media (max-width: 900px) {
      .dashboard-area { max-width: 99vw; }
      .card-row { flex-direction: column; gap: 18px; }
      .card { width: 88vw; min-width: 0; padding: 16px 4vw 12px 4vw; }
      .chart-canvas { max-width: 98vw; }
    }
    @media (max-width: 600px) {
      h2 { font-size: 1.25em; }
      .dashboard-area { padding-bottom: 8px; }
      .card { padding: 8px 2vw 8px 2vw; }
      canvas, .chart-canvas { height: 80px !important; width: 85vw !important; max-width: 97vw; }
      .gauge-value { font-size: 1.2em; }
      .per-room-data { gap: 8px; }
      .per-room-card { padding: 6px 4vw 5px 4vw; }
    }
  </style>
</head>
<body>
  <h2>Room Climate Dashboard</h2>
  <div class="dashboard-area">
    <div class="per-room-data" id="perRoomData">
      <!-- Per-room data cards inserted here -->
    </div>
    <!-- Gauges in cards -->
    <div class="card-row">
      <div class="card">
        <div class="gauge-label" style="color:#f15454;">Temperature (&deg;C) (Avg)</div>
        <canvas id="tempMeter"></canvas>
        <div id="tempVal" class="gauge-value" style="color:#f15454;">--</div>
      </div>
      <div class="card">
        <div class="gauge-label" style="color:#3584e4;">Humidity (%) (Avg)</div>
        <canvas id="humMeter"></canvas>
        <div id="humVal" class="gauge-value" style="color:#3584e4;">--</div>
      </div>
    </div>
    <!-- Charts in cards -->
    <div class="card-row">
      <div class="card">
        <div class="chart-label">Temperature Trend</div>
        <canvas id="tempChart" class="chart-canvas"></canvas>
      </div>
      <div class="card">
        <div class="chart-label">Humidity Trend</div>
        <canvas id="humChart" class="chart-canvas"></canvas>
      </div>
    </div>
  </div>
  <script>
    // Gague.js options
    var tempGauge, humGauge;
    function createGauges() {
      var tempOpts = {
        angle: 0, lineWidth: 0.32, radiusScale: 1,
        pointer: {length: 0.65, strokeWidth: 0.035, color: '#f15454'},
        limitMax: false, limitMin: false, colorStart: '#fff2f2', colorStop: '#f15454',
        strokeColor: '#e0e0e0', generateGradient: true, highDpiSupport: true,
        staticZones: [
          {strokeStyle: "#d0f0ff", min: 0, max: 20},
          {strokeStyle: "#ffe066", min: 20, max: 30},
          {strokeStyle: "#ff6f69", min: 30, max: 50}
        ],
        staticLabels: {
          font: "13px Segoe UI",
          labels: [0, 10, 20, 30, 40, 50],
          color: "#f15454",
          fractionDigits: 0
        },
        renderTicks: {
          divisions: 5, divWidth: 1.1, divLength: 0.42, divColor: "#333333",
          subDivisions: 2, subLength: 0.22, subWidth: 0.7, subColor: "#aaa"
        }
      };
      var humOpts = {
        angle: 0, lineWidth: 0.32, radiusScale: 1,
        pointer: {length: 0.65, strokeWidth: 0.035, color: '#3584e4'},
        limitMax: false, limitMin: false, colorStart: '#e3f2fd', colorStop: '#3584e4',
        strokeColor: '#e0e0e0', generateGradient: true, highDpiSupport: true,
        staticZones: [
          {strokeStyle: "#d0e6ff", min: 0, max: 30},
          {strokeStyle: "#a7f3d0", min: 30, max: 70},
          {strokeStyle: "#ffa8a8", min: 70, max: 100}
        ],
        staticLabels: {
          font: "13px Segoe UI",
          labels: [0, 20, 40, 60, 80, 100],
          color: "#3584e4",
          fractionDigits: 0
        },
        renderTicks: {
          divisions: 5, divWidth: 1.1, divLength: 0.42, divColor: "#333333",
          subDivisions: 2, subLength: 0.22, subWidth: 0.7, subColor: "#aaa"
        }
      };
      tempGauge = new Gauge(document.getElementById('tempMeter')).setOptions(tempOpts);
      tempGauge.maxValue = 50; tempGauge.setMinValue(0); tempGauge.animationSpeed = 46; tempGauge.set(0);
      humGauge = new Gauge(document.getElementById('humMeter')).setOptions(humOpts);
      humGauge.maxValue = 100; humGauge.setMinValue(0); humGauge.animationSpeed = 46; humGauge.set(0);
    }
    createGauges();

    let tempChart, humChart;

    async function fetchData() {
      const res = await fetch('/data');
      return await res.json();
    }

    function updatePerRoom(data) {
      let arr = data.clients || [];
      let html = "";
      for (let i = 0; i < arr.length; i++) {
        let n = arr[i].client_name || ("Sensor " + (i + 1));
        let t = arr[i].temperature;
        let h = arr[i].humidity;
        html += `<div class="per-room-card">
          <div class="per-room-title">${n}</div>
          <div class="per-room-value per-room-temp">🌡️ ${isNaN(t) ? '--' : t + '°C'}</div>
          <div class="per-room-value per-room-hum">💧 ${isNaN(h) ? '--' : h + '%'}</div>
        </div>`;
      }
      document.getElementById("perRoomData").innerHTML = html;
    }

    async function updateDashboard() {
      const data = await fetchData();
      let t = Math.round(data.currentTemperature*10)/10;
      let h = Math.round(data.currentHumidity*10)/10;
      document.getElementById('tempVal').innerText = isNaN(t) ? '--' : t+'°C';
      document.getElementById('humVal').innerText = isNaN(h) ? '--' : h+'%';
      tempGauge.set(isNaN(t)?0:t);
      humGauge.set(isNaN(h)?0:h);

      // Update per-room
      updatePerRoom(data);

      // Graphs
      const labels = data.timestamps.map(t => t + "s");
      const tempCtx = document.getElementById('tempChart').getContext('2d');
      const humCtx = document.getElementById('humChart').getContext('2d');
      if(tempChart) tempChart.destroy();
      if(humChart) humChart.destroy();
      tempChart = new Chart(tempCtx, {
        type: 'line',
        data: {
          labels: labels,
          datasets: [{
            label: 'Temperature (°C)',
            data: data.temperature,
            borderColor: '#f15454',
            backgroundColor: 'rgba(224,74,74,0.10)',
            fill: true,
            tension: 0.32,
            pointRadius: 1.5,
          }]
        },
        options: {
          responsive: true,
          plugins: { legend: { display: false }, title: { display: false } },
          elements: { line: { borderWidth: 2 } },
          scales: { y: { title: { display:true, text: "°C" }, min:0, max:50, ticks:{font:{size:10}} },
                    x: { ticks:{font:{size:10}} }
          }
        }
      });
      humChart = new Chart(humCtx, {
        type: 'line',
        data: {
          labels: labels,
          datasets: [{
            label: 'Humidity (%)',
            data: data.humidity,
            borderColor: '#3584e4',
            backgroundColor: 'rgba(53,132,228,0.10)',
            fill: true,
            tension: 0.32,
            pointRadius: 1.5,
          }]
        },
        options: {
          responsive: true,
          plugins: { legend: { display: false }, title: { display: false } },
          elements: { line: { borderWidth: 2 } },
          scales: { y: { title: { display:true, text: "%" }, min:0, max:100, ticks:{font:{size:10}} },
                    x: { ticks:{font:{size:10}} }
          }
        }
      });
    }

    updateDashboard();
    setInterval(updateDashboard, 4000);
  </script>
</body>
</html>
      )rawliteral");
    } else {
      // 404 Not Found
      client.println("HTTP/1.1 404 Not Found");
      client.println("Connection: close");
      client.println();
    }
    delay(5);
    client.stop();
  }
}
