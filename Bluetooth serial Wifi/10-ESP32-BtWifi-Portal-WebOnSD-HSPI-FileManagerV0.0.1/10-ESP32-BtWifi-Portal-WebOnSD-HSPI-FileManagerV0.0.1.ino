/*
features:
  - wifi
  - bluetooth
  - sd card
  - html
  - html on sd
  - bluetooth and sd at same time
  - file manager
todo:
  - combine portal with filemanager
  - move file manager to sd
  - 
*/


#include <WiFi.h>
#include <WebServer.h>
#include <SPI.h>
//#include "mySD.h"
#include <SD.h>
#include <BluetoothSerial.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include "file_manager_html.h" // include the html from PROGMEM
#include "html_Page_Internal.h" // include internal html page 

#define SD_CS 5
SPIClass hspi(HSPI);

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

File fsUploadFile;

// --- List files in JSON format ---
void handleList() {
  File root = SD.open("/");
  String json = "[";
  while (true) {
    File file = root.openNextFile();
    if (!file) break;
    if (json.length() > 1) json += ",";
    json += "{\"name\":\"" + String(file.name()) + "\",\"size\":" + String(file.size()) + "}";
    file.close();
  }
  json += "]";
  server.send(200, "application/json", json);
}

// --- Upload handler ---
void handleUpload() {
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    String filename = "/" + upload.filename;
    fsUploadFile = SD.open(filename, FILE_WRITE);
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (fsUploadFile) fsUploadFile.write(upload.buf, upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_END) {
    if (fsUploadFile) fsUploadFile.close();
    server.sendHeader("Location", "/");
    server.send(303);
  }
}

// --- Delete handler ---
void handleDelete() {
  if (!server.hasArg("file")) {
    server.send(400, "text/plain", "Missing file name");
    return;
  }
  String path = "/" + server.arg("file");
  if (!SD.exists(path)) {
    server.send(404, "text/plain", "File not found");
    return;
  }
  SD.remove(path);
  server.send(200, "text/plain", "File deleted");
}

// --- Rename handler ---
void handleRename() {
  if (!server.hasArg("old") || !server.hasArg("new")) {
    server.send(400, "text/plain", "Missing arguments");
    return;
  }
  String oldPath = "/" + server.arg("old");
  String newPath = "/" + server.arg("new");
  if (!SD.exists(oldPath)) {
    server.send(404, "text/plain", "Old file not found");
    return;
  }
  if (SD.exists(newPath)) SD.remove(newPath);
  SD.rename(oldPath, newPath);
  server.send(200, "text/plain", "Renamed");
}

// --- Download file ---
void handleDownload() {
  if (!server.hasArg("file")) {
    server.send(400, "text/plain", "Missing file name");
    return;
  }
  String path = "/" + server.arg("file");
  if (!SD.exists(path)) {
    server.send(404, "text/plain", "File not found");
    return;
  }
  File downloadFile = SD.open(path);
  server.streamFile(downloadFile, "application/octet-stream");
  downloadFile.close();
}

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
  Serial.begin(115200);
  delay(300);

  WiFi.begin(ssid, password);
  //Serial.print("Connecting to WiFi");
  Serial.printf("Connecting to %s", ssid);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ WiFi connected: " + WiFi.localIP().toString());
  delay(300);

  // Routes
  server.on("/", handleRoot);
  server.on("/int", handleRootInternal);
  server.on("/styles.css", handleCSS);
  server.on("/script.js", handleJS);
  server.on("/chart.min.js", handleChartMinJS);
  server.on("/chart.js", handleChartJS);
  // file manager routes
  server.on("/fm", []() { server.send(200, "text/html", fileManagerHtml); });
  server.on("/list", HTTP_GET, handleList);
  server.on("/upload", HTTP_POST, []() { server.send(200); }, handleUpload);
  server.on("/delete", HTTP_GET, handleDelete);
  server.on("/rename", HTTP_GET, handleRename);
  server.on("/download", HTTP_GET, handleDownload);

  server.begin();
  Serial.println("📡 Web server started.");
  delay(300);

  // Start websocket
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.println("📡 Websocket started.");
  delay(300);

  // HSPI sd-card init
  hspi.begin(14, 12, 13, SD_CS);
  delay(300);
  // if (!SD.beginnn(SD_CS, hspi)) {
  if (!SD.begin(SD_CS, hspi)) {
    Serial.println("❌ SD init failed");
    return;
  }
  Serial.println("✅ SD initialized");
  delay(300);

  // Start bluetooth
  //Serial.println("starting bluetooth...");
  delay(3333);
  SerialBT.begin("Hila_ECG_Monitor");  // Bluetooth device name
  SerialBT.register_callback(bluetoothCallback); // Register Bluetooth callback
  Serial.println("✅Bluetooth Started.");
  delay(222);

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
