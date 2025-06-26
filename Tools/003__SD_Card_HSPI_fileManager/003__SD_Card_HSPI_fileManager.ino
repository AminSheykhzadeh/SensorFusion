#include <WiFi.h>
#include <WebServer.h>
#include <SPI.h>
#include <SD.h>

#define SD_CS 5
SPIClass hspi(HSPI);
WebServer server(80);

const char* ssid = "AminJoon";
const char* password = "123456789";

File fsUploadFile;

// --- HTML File Manager ---
const char indexHtml[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <title>ESP32 File Manager</title>
  <style>
    body { font-family: sans-serif; text-align: center; background: #f7f7f7; }
    table { margin: auto; border-collapse: collapse; width: 90%; }
    th, td { border: 1px solid #ccc; padding: 8px; }
    th { background-color: #f0f0f0; }
    button { padding: 4px 8px; margin: 2px; }
    input { margin: 4px; }
  </style>
</head>
<body>
  <h2>ESP32 File Manager</h2>
  <table id="fileTable">
    <tr><th>Name</th><th>Size (bytes)</th><th>Actions</th></tr>
  </table>
  <br>
  <h3>Upload File</h3>
  <form method="POST" action="/upload" enctype="multipart/form-data">
    <input type="file" name="file"><br><br>
    <input type="submit" value="Upload">
  </form>
<script>
function loadFiles() {
  fetch('/list').then(r => r.json()).then(data => {
    let table = document.getElementById("fileTable");
    table.innerHTML = '<tr><th>Name</th><th>Size (bytes)</th><th>Actions</th></tr>';
    data.forEach(f => {
      let row = `<tr>
        <td>${f.name}</td>
        <td>${f.size}</td>
        <td>
          <a href="/download?file=${f.name}">Download</a>
          <button onclick="deleteFile('${f.name}')">Delete</button>
          <button onclick="renameFile('${f.name}')">Rename</button>
        </td>
      </tr>`;
      table.innerHTML += row;
    });
  });
}
function deleteFile(name) {
  fetch('/delete?file=' + name).then(() => loadFiles());
}
function renameFile(oldName) {
  let newName = prompt("New name:", oldName);
  if (newName && newName !== oldName) {
    fetch(`/rename?old=${oldName}&new=${newName}`).then(() => loadFiles());
  }
}
loadFiles();
</script>
</body>
</html>
)rawliteral";

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

void setup() {
  Serial.begin(115200);

  // HSPI init
  hspi.begin(14, 12, 13, SD_CS);
  if (!SD.begin(SD_CS, hspi)) {
    Serial.println("❌ SD init failed");
    return;
  }
  Serial.println("✅ SD initialized");

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ WiFi connected: " + WiFi.localIP().toString());

  // Routes
  server.on("/", []() { server.send(200, "text/html", indexHtml); });
  server.on("/list", HTTP_GET, handleList);
  server.on("/upload", HTTP_POST, []() { server.send(200); }, handleUpload);
  server.on("/delete", HTTP_GET, handleDelete);
  server.on("/rename", HTTP_GET, handleRename);
  server.on("/download", HTTP_GET, handleDownload);

  server.begin();
  Serial.println("📡 Web server started.");
}

void loop() {
  server.handleClient();
}

/*
// ESP32 File Manager Server (Handles HTML interface)
#include <WiFi.h>
#include <WebServer.h>
#include <SPI.h>
#include <SD.h>

#define SD_CS_PIN 5
const char* ssid = "AminJoon";
const char* password = "123456789";
WebServer server(80);

// Helper: send JSON file list
void handleList() {
  File root = SD.open("/");
  String json = "[";
  File entry;
  while ((entry = root.openNextFile())) {
    if (json != "[") json += ",";
    json += "{\"name\":\"" + String(entry.name()) + "\",";
    json += "\"size\":" + String(entry.size()) + ",";
    json += "\"isDir\":" + String(entry.isDirectory() ? "true" : "false") + "}";
    entry.close();
  }
  json += "]";
  server.send(200, "application/json", json);
}

// Helper: delete file
void handleDelete() {
  String file = server.arg("file");
  if (SD.exists(file.c_str())) {
    SD.remove(file.c_str());
    server.send(200, "text/plain", "Deleted");
  } else {
    server.send(404, "text/plain", "File not found");
  }
}

// Helper: rename file
void handleRename() {
  String oldName = server.arg("old");
  String newName = server.arg("new");
  if (SD.exists(oldName.c_str())) {
    if (SD.rename(oldName.c_str(), newName.c_str()))
      server.send(200, "text/plain", "Renamed");
    else
      server.send(500, "text/plain", "Rename failed");
  } else {
    server.send(404, "text/plain", "Original file not found");
  }
}

// Helper: download file
void handleDownload() {
  String fileName = server.arg("file");
  File file = SD.open(fileName.c_str());
  if (!file) {
    server.send(404, "text/plain", "File not found");
    return;
  }
  server.streamFile(file, "application/octet-stream");
  file.close();
}

// Helper: upload file
File uploadFile;
void handleUpload() {
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    String filename = "/" + upload.filename;
    uploadFile = SD.open(filename.c_str(), FILE_WRITE);
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (uploadFile)
      uploadFile.write(upload.buf, upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_END) {
    if (uploadFile) uploadFile.close();
  }
}

// Serve main HTML
void handleRoot() {
  File file = SD.open("/filemanager.html");
  if (!file) {
    server.send(500, "text/plain", "filemanager.html not found on SD");
    return;
  }
  server.streamFile(file, "text/html");
  file.close();
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");

  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("SD init failed");
    return;
  }
  Serial.println("SD init OK");

  server.on("/", handleRoot);
  server.on("/list", handleList);
  server.on("/delete", handleDelete);
  server.on("/rename", handleRename);
  server.on("/download", handleDownload);
  server.on("/upload", HTTP_POST, []() { server.send(200); }, handleUpload);

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
}

*/
