#include <BluetoothSerial.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <SD.h>

// SD card CS pin (adjust based on your wiring)
#define SD_CS_PIN 33

// WiFi credentials
const char* ssid = "AminJoon";      // Replace with your WiFi SSID
const char* password = "12345678";  // Replace with your WiFi password

BluetoothSerial SerialBT;
WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

String receivedData = "";
const int maxDataPoints = 50;             // Number of data points to display on the chart
float dataValues[maxDataPoints] = { 0 };  // Array to store data for plotting
int dataIndex = 0;
int connectedWebSocketClients = 0;        // Track WebSocket clients
bool bluetoothConnected = false;          // Track Bluetooth connection status

// HTML page with Chart.js and connection statuses
  const char* htmlPageInternal = R"rawliteral(
  <!DOCTYPE html>
  <html>
  <head>
    <title>ESP32 Bluetooth Data Plot</title>
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
    <style>
      body { font-family: Arial, sans-serif; text-align: center; }
      #dataDisplay { margin: 20px; font-size: 20px; }
      #connectionStatus, #bluetoothStatus { margin: 10px; font-size: 16px; }
      #connectionStatus { color: red; }
      #bluetoothStatus { color: red; }
      #chartContainer { width: 80%; margin: auto; }
    </style>
  </head>
  <body>
    <h1>ESP32 Bluetooth Serial Data</h1>
    <br>
    <h1>هیوا | حس خوب زندگی</h1>
    <br>
    <h3>شرکت سجنش طب هیلا :)</h3>
    <div id="connectionStatus">No WebSocket Clients Connected</div>
    <div id="bluetoothStatus">Bluetooth Disconnected</div>
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
        document.getElementById('connectionStatus').style.color = 'green';
      };

      ws.onmessage = function(event) {
        let data = JSON.parse(event.data);
        if (data.type === 'connection') {
          if (data.device === 'websocket') {
            document.getElementById('connectionStatus').innerText = data.message;
            document.getElementById('connectionStatus').style.color = data.connected ? 'green' : 'red';
          } else if (data.device === 'bluetooth') {
            document.getElementById('bluetoothStatus').innerText = data.message;
            document.getElementById('bluetoothStatus').style.color = data.connected ? 'green' : 'red';
          }
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
        document.getElementById('connectionStatus').innerText = 'No WebSocket Clients Connected';
        document.getElementById('connectionStatus').style.color = 'red';
        ws = new WebSocket('ws://' + window.location.hostname + ':81/');
      };
    </script>
  </body>
  </html>
  )rawliteral";

void handleRootInternal() {
  server.send(200, "text/html", htmlPageInternal);
}

void handleRoot() {
  File file = SD.open("/index.html");
  if (file) {
    server.streamFile(file, "text/html");
    file.close();
  } else {
    server.send(404, "text/plain", "File not found: /index.html");
    Serial.println("Error: /index.html not found on SD card");
  }
}


void handleCSS() {
  File file = SD.open("/styles.css");
  if (file) {
    server.streamFile(file, "text/css");
    file.close();
  } else {
    server.send(404, "text/plain", "File not found: /styles.css");
    Serial.println("Error: /styles.css not found on SD card");
  }
}

void handleJS() {
  File file = SD.open("/script.js");
  if (file) {
    server.streamFile(file, "application/javascript");
    file.close();
  } else {
    server.send(404, "text/plain", "File not found: /script.js");
    Serial.println("Error: /script.js not found on SD card");
  }
}

void handleChartJS() {
  File file = SD.open("/chart.min.js");
  if (file) {
    server.streamFile(file, "application/javascript");
    file.close();
  } else {
    server.send(404, "text/plain", "File not found: /chart.min.js");
    Serial.println("Error: /chart.min.js not found on SD card");
  }
}

// Bluetooth connection callback
void bluetoothCallback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
  if (event == ESP_SPP_SRV_OPEN_EVT) {
    bluetoothConnected = true;
    Serial.println("Bluetooth Client Connected");
    
    // Broadcast Bluetooth connection status
    DynamicJsonDocument doc(200);
    doc["type"] = "connection";
    doc["device"] = "bluetooth";
    doc["message"] = "Bluetooth Connected";
    doc["connected"] = true;
    String json;
    serializeJson(doc, json);
    webSocket.broadcastTXT(json);
  } else if (event == ESP_SPP_CLOSE_EVT) {
    bluetoothConnected = false;
    Serial.println("Bluetooth Client Disconnected");
    
    // Broadcast Bluetooth disconnection status
    DynamicJsonDocument doc(200);
    doc["type"] = "connection";
    doc["device"] = "bluetooth";
    doc["message"] = "Bluetooth Disconnected";
    doc["connected"] = false;
    String json;
    serializeJson(doc, json);
    webSocket.broadcastTXT(json);
  }
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {
    case WStype_CONNECTED: {
      connectedWebSocketClients++;
      Serial.printf("WebSocket Client %u connected from %s\n", num, webSocket.remoteIP(num).toString().c_str());
      
      // Send WebSocket connection status to all clients
      DynamicJsonDocument doc(200);
      doc["type"] = "connection";
      doc["device"] = "websocket";
      doc["message"] = String("WebSocket Clients Connected: ") + connectedWebSocketClients;
      doc["connected"] = true;
      String json;
      serializeJson(doc, json);
      webSocket.broadcastTXT(json);

      // Send current Bluetooth status to the new client
      DynamicJsonDocument btDoc(200);
      btDoc["type"] = "connection";
      btDoc["device"] = "bluetooth";
      btDoc["message"] = bluetoothConnected ? "Bluetooth Connected" : "Bluetooth Disconnected";
      btDoc["connected"] = bluetoothConnected;
      String btJson;
      serializeJson(btDoc, btJson);
      webSocket.sendTXT(num, btJson); // Send only to the new client
      break;
    }
    case WStype_DISCONNECTED: {
      connectedWebSocketClients--;
      Serial.printf("WebSocket Client %u disconnected\n", num);
      
      // Broadcast WebSocket disconnection status
      DynamicJsonDocument doc(200);
      doc["type"] = "connection";
      doc["device"] = "websocket";
      doc["message"] = connectedWebSocketClients > 0 ? String("WebSocket Clients Connected: ") + connectedWebSocketClients : "No WebSocket Clients Connected";
      doc["connected"] = connectedWebSocketClients > 0;
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
  Serial.begin(115200); delay(50);
  SerialBT.begin("ESP32_Bluetooth"); delay(50);  // Bluetooth device name 
  SerialBT.register_callback(bluetoothCallback); // Register Bluetooth callback
  Serial.println("Bluetooth Started. Pair with ESP32_Bluetooth");

  // Connect to WiFi
  WiFi.begin(ssid, password); delay(50);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.println(WiFi.localIP());

  Initialize SD card
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("SD Card initialization failed!");
    return;
  }
  Serial.println("SD Card initialized");
  delay(1000);


  // Start web server
  server.on("/", handleRoot);
  server.on("/int", handleRootInternal);
  
  server.begin();
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

void loop() {
  server.handleClient();
  webSocket.loop();

      // check WiFi
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.disconnect(true);  // Disconnect from WiFi and delete saved credentials
    delay(100);
    WiFi.mode(WIFI_OFF);    // Turn off WiFi hardware
    delay(100);
    WiFi.mode(WIFI_STA);         // Set mode to Station
    WiFi.begin(ssid, password); // Reconnect
    delay(1000);
    Serial.println("Connecting to WiFi...");
  
    while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  
  Serial.println("Connected to WiFi");
  Serial.println(WiFi.localIP());
  }

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