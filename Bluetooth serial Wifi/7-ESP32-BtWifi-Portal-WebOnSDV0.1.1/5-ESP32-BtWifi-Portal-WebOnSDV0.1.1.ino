//working

#include <BluetoothSerial.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <SD.h>

#define SD_CS_PIN 5  // used for sd card cs pin, you can change it

const char* ssid = "AminJoon";
const char* password = "123456789";

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
    <meta charset="UTF-8">
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
    <h1>ESP32 Bluetooth Serial Data (internal memory!)</h1>
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

// Function: Handle root (internal memory)
void handleRootInternal() {
  server.send(200, "text/html", htmlPageInternal);
}

// Function: Handle root (SD-Card)
void handleRoot() {
  File file = SD.open("/index.html");
  if (!file) {
    server.send(500, "text/plain", "Failed to open index.html");
    file.close();
    return;
  }

  server.streamFile(file, "text/html");
  Serial.println("server streamFile html");
  file.close();
}

// Function: Send file: CSS
void handleCSS() {
  File file = SD.open("/styles.css");
  if (!file) {
    server.send(500, "text/plain", "Failed to open styles.css");
    Serial.println("Error: /styles.css not found on SD card");
    file.close();
    return;
  }

  //server.streamFile(file, "text/html");
  server.streamFile(file, "text/css");
  Serial.println("server streamFile CSS");
  file.close();
}

// Function: Send file: JS
void handleJS() {
  File file = SD.open("/script.js");
  if (!file) {
    server.send(500, "text/plain", "Failed to open script.js"); //Err: 404
    Serial.println("Error: /script.js not found on SD card");
    file.close();
    return;
  }

  //server.streamFile(file, "text/html");
  server.streamFile(file, "application/javascript");
  // server.streamFile(file, "text/js");
  Serial.println("server streamFile JS");
  file.close();
}

// Function: Send file: ChartJS (minimal)
void handleChartMinJS() {
  File file = SD.open("/chart.js");
  if (!file) {
    server.send(500, "text/plain", "File not found: /chart.min.js");
    Serial.println("Error: /chart.min.js not found on SD card");
    file.close();
    return;
  }

  //server.streamFile(file, "text/html");
  server.streamFile(file, "application/javascript");
  // server.streamFile(file, "text/js");
  Serial.println("server streamFile ChartMinJS");
  file.close();
}

// Function: Send file: ChartJS
void handleChartJS() {
  File file = SD.open("/chart.js");
  if (!file) {
    server.send(500, "text/plain", "File not found: /chart.js");
    Serial.println("Error: /chart.js not found on SD card");
    file.close();
    return;
  }

  //server.streamFile(file, "text/html");
  server.streamFile(file, "application/javascript");
  // server.streamFile(file, "text/js");
  Serial.println("server streamFile ChartJS");
  file.close();
}

void setup() {
  Serial.begin(115200);
  delay(300);

  // Initialize SD
  Serial.println("Initializing SD...");
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("SD failed!");
    return;
  }
  Serial.println("SD initialized.");
  delay(300);
  
  // Connect to WiFi
  Serial.printf("Connecting to %s", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.println(WiFi.localIP());
  
  // Set up web server routes
  server.on("/", handleRoot);
  server.on("/int", handleRootInternal);
  server.on("/styles.css", handleCSS);
  server.on("/script.js", handleJS);
  server.on("/chart.min.js", handleChartMinJS);
  server.on("/chart.js", handleChartJS);
  
  // Start server
  server.begin();
  Serial.println("HTTP server started.");
}

void loop() {
  server.handleClient();
}


/*

#include <WiFi.h>
#include <SPI.h>
#include <SD.h>

#define SD_CS_PIN 5  

const char* ssid = "HilaWifi";
const char* password = "12345678";

void setup() {
  Serial.begin(115200);

  // Initialize SD first
  Serial.println("Initializing SD card...");
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("❌ SD Card initialization failed!");
    return;
  }
  Serial.println("✅ SD Card OK");

  // Delay before starting WiFi (prevent SPI conflict)
  delay(100);

  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ WiFi Connected!");

  // List files (optional)
  File root = SD.open("/");
  printDirectory(root, 0);
  root.close();
}

void loop() {
  // خواندن یا نوشتن روی SD کارت
}

void printDirectory(File dir, int numTabs) {
  while (true) {
    File entry = dir.openNextFile();
    if (!entry) break;

    for (uint8_t i = 0; i < numTabs; i++) Serial.print('\t');

    Serial.print(entry.name());
    if (entry.isDirectory()) {
      Serial.println("/");
      printDirectory(entry, numTabs + 1);
    } else {
      Serial.print("\t\t");
      Serial.println(entry.size(), DEC);
    }
    entry.close();
  }
}

*/