#include <BluetoothSerial.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>

// WiFi credentials
const char* ssid = "HilaWifi";      // Replace with your WiFi SSID
const char* password = "12345678";  // Replace with your WiFi password

BluetoothSerial SerialBT;
WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

String receivedData = "";
const int maxDataPoints = 10;             // Number of data points to display on the chart
float dataValues[maxDataPoints] = { 0 };  // Array to store data for plotting
int dataIndex = 0;
int connectedClients = 0;                // Track number of connected WebSocket clients

// HTML page with Chart.js and connection status
const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>ESP32 Bluetooth Data Plot</title>
  <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
  <style>
    body { font-family: Arial, sans-serif; text-align: center; }
    #dataDisplay { margin: 20px; font-size: 20px; }
    #connectionStatus { margin: 10px; font-size: 16px; color: green; }
    #chartContainer { width: 80%; margin: auto; }
  </style>
</head>
<body>
  <h1>ESP32 Bluetooth Serial Data</h1>
  <div id="connectionStatus">WebSocket Disconnected</div>
  <div id="dataDisplay">Latest Data: Waiting for data...</div>
  <div id="chartContainer">
    <canvas id="dataChart"></canvas>
  </div>
  <script>
    let ws = new WebSocket('ws://' + window.location.hostname + ':81/');
    let chartData = { labels: [], datasets: [{ label: 'Bluetooth Data', data: [], borderColor: 'blue', fill: false }] };
    let chart = new Chart(document.getElementById('dataChart'), {
      type: 'line',
      data: chartData,
      options: { scales: { x: { title: { display: true, text: 'Sample' } }, y: { title: { display: true, text: 'Value' } } } }
    });

    ws.onopen = function() {
      document.getElementById('connectionStatus').innerText = 'WebSocket Connected';
      document.getElementById('connectionStatus').style.color = 'green';
    };

    ws.onmessage = function(event) {
      let data = JSON.parse(event.data);
      if (data.type === 'connection') {
        document.getElementById('connectionStatus').innerText = data.message;
        document.getElementById('connectionStatus').style.color = data.connected ? 'green' : 'red';
      } else {
        document.getElementById('dataDisplay').innerText = 'Latest Data: ' + data.value;
        chartData.labels.push(data.index);
        chartData.datasets[0].data.push(parseFloat(data.value));
        if (chartData.labels.length > 50) {
          chartData.labels.shift();
          chartData.datasets[0].data.shift();
        }
        chart.update();
      }
    };

    ws.onclose = function() {
      document.getElementById('connectionStatus').innerText = 'WebSocket Disconnected';
      document.getElementById('connectionStatus').style.color = 'red';
      ws = new WebSocket('ws://' + window.location.hostname + ':81/');
    };
  </script>
</body>
</html>
)rawliteral";

void handleRoot() {
  server.send(200, "text/html", htmlPage);
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {
    case WStype_CONNECTED: {
      connectedClients++;
      Serial.printf("WebSocket Client %u connected from %s\n", num, webSocket.remoteIP(num).toString().c_str());
      
      // Send connection status to all clients
      DynamicJsonDocument doc(200);
      doc["type"] = "connection";
      doc["message"] = String("WebSocket Clients Connected: ") + connectedClients;
      doc["connected"] = true;
      String json;
      serializeJson(doc, json);
      webSocket.broadcastTXT(json);
      break;
    }
    case WStype_DISCONNECTED: {
      connectedClients--;
      Serial.printf("WebSocket Client %u disconnected\n", num);
      
      // Send updated connection status
      DynamicJsonDocument doc(200);
      doc["type"] = "connection";
      doc["message"] = connectedClients > 0 ? String("WebSocket Clients Connected: ") + connectedClients : "No WebSocket Clients Connected";
      doc["connected"] = connectedClients > 0;
      String json;
      serializeJson(doc, json);
      webSocket.broadcastTXT(json);
      break;
    }
    case WStype_TEXT:
      // Handle other WebSocket messages if needed
      break;
  }
}

void setup() {
  Serial.begin(115200);
  SerialBT.begin("Hila_BT_Portal");  // Bluetooth device name
  Serial.println("Bluetooth Started. Pair with ESP32_Bluetooth");

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.println(WiFi.localIP());

  // Start web server
  server.on("/", handleRoot);
  server.begin();
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

void loop() {
  server.handleClient();
  webSocket.loop();

  // Handle Bluetooth data
  if (SerialBT.available()) {
    receivedData = SerialBT.readStringUntil('\n');
    receivedData.trim();
    if (receivedData.length() > 0) {
      Serial.println("Received: " + receivedData);

      // Try to parse the received data as a float
      float value = receivedData.toFloat();
      dataValues[dataIndex % maxDataPoints] = value;
      dataIndex++;

      // Send data to WebSocket clients
      DynamicJsonDocument doc(200);
      doc["index"] = dataIndex;
      doc["value"] = value;
      String json;
      serializeJson(doc, json);
      webSocket.broadcastTXT(json);
    }
  }
}