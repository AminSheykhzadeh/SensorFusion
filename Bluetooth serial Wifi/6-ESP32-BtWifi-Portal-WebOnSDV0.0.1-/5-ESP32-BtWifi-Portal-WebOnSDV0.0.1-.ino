#include <BluetoothSerial.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <SD.h>

// WiFi credentials
const char* ssid = "HilaWifi";      // Replace with your WiFi SSID
const char* password = "12345678";  // Replace with your WiFi password

// SD card CS pin (adjust based on your wiring)
#define SD_CS_PIN 5

BluetoothSerial SerialBT;
WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

String receivedData = "";
const int maxDataPoints = 50;             // Number of data points to display
float dataValues[maxDataPoints] = { 0 };  // Array to store data for plotting
int dataIndex = 0;
int connectedWebSocketClients = 0;        // Track WebSocket clients
bool bluetoothConnected = false;          // Track Bluetooth connection status

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
      webSocket.sendTXT(num, btJson);
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
  Serial.begin(115200);
  delay(1000);
  SerialBT.begin("ESP32_Bluetooth");  // Bluetooth device name
  SerialBT.register_callback(bluetoothCallback);
  Serial.println("Bluetooth Started. Pair with ESP32_Bluetooth");
  delay(1000);

  // Connect to WiFi with timeout
  WiFi.begin(ssid, password);
  unsigned long startAttemptTime = millis();
  // const unsigned long timeout = 30000; // 30 seconds timeout
  // while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < timeout) {
  //   delay(1000);
  //   Serial.println("Connecting to WiFi...");
  // }
  delay(3000);
  while (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(ssid, password);
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Failed to connect to WiFi. Check credentials or signal.");
    Serial.println("MAC Address: " + WiFi.macAddress());
    return;
  }
  Serial.println("Connected to WiFi");
  Serial.println("IP Address: " + WiFi.localIP().toString());
  Serial.println("MAC Address: " + WiFi.macAddress());
  delay(1000);


    // Initialize SD card
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("SD Card initialization failed!");
    return;
  }
  Serial.println("SD Card initialized");
  delay(1000);

  // Set up web server routes
  server.on("/", handleRoot);
  server.on("/styles.css", handleCSS);
  server.on("/script.js", handleJS);
  server.on("/chart.min.js", handleChartJS);
  server.begin();
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    server.handleClient();
    webSocket.loop();
  } else {
    Serial.println("WiFi disconnected. Attempting to reconnect...");
    WiFi.reconnect();
    delay(5000);
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
      DynamicJsonDocument doc(64);
      doc["index"] = dataIndex;
      doc["value"] = value;
      String json;
      serializeJson(doc, json);
      webSocket.broadcastTXT(json);
    }
  }
}