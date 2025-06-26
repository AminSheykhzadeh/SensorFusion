#include <WiFi.h>
#include <SPI.h>
#include <SD.h>
#include <FS.h>
#include <WebServer.h>
#include "file_manager_html.h" // include the html from PROGMEM

#define SD_CS 5 // یا پین مربوط به CS کارت SD در HSPI

WebServer server(80);

// WiFi credentials
const char* ssid = "AminJoon";
const char* password = "123456789";

SPIClass spiHSPI(VSPI);

void listDir(const String& path) {
  File root = SD.open(path);
  if (!root || !root.isDirectory()) return;

  File file = root.openNextFile();
  while (file) {
    Serial.print(file.isDirectory() ? "[DIR] " : "[FILE] ");
    Serial.print(file.name());
    if (!file.isDirectory()) {
      Serial.print(" \t");
      Serial.println(file.size());
    } else {
      Serial.println();
    }
    file = root.openNextFile();
  }
}

void handleRoot() {
  server.send_P(200, "text/html", fileManagerHtml);
}

void handleList() {
  String path = server.hasArg("dir") ? server.arg("dir") : "/";
  File root = SD.open(path);
  if (!root || !root.isDirectory()) {
    server.send(500, "text/plain", "Failed to open directory");
    return;
  }

  String output = "[";
  File file = root.openNextFile();
  bool firstItem = true;

  while (file) {
    if (!firstItem) output += ',';
    output += '{';
    output += "\"name\":\"" + String(file.name()).substring(path.length()) + "\",";
    output += "\"size\":" + String(file.size()) + ",";
    output += "\"type\":\"" + String(file.isDirectory() ? "dir" : "file") + "\"";
    output += '}';
    firstItem = false;
    file = root.openNextFile();
  }
  output += "]";
  server.send(200, "application/json", output);
}

void handleUpload() {
  HTTPUpload& upload = server.upload();
  static File fsUploadFile;

  if (upload.status == UPLOAD_FILE_START) {
    String filename = "/" + upload.filename;
    if (server.hasArg("dir")) filename = server.arg("dir") + "/" + upload.filename;
    fsUploadFile = SD.open(filename, FILE_WRITE);
  } else if (upload.status == UPLOAD_FILE_WRITE && fsUploadFile) {
    fsUploadFile.write(upload.buf, upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_END && fsUploadFile) {
    fsUploadFile.close();
    server.send(200, "text/plain", "Upload complete");
  }
}

void handleDelete() {
  if (!server.hasArg("path")) {
    server.send(400, "text/plain", "Missing path");
    return;
  }
  String path = server.arg("path");
  if (SD.exists(path)) {
    File f = SD.open(path);
    if (f.isDirectory()) {
      SD.rmdir(path);
    } else {
      SD.remove(path);
    }
    server.send(200, "text/plain", "Deleted");
  } else {
    server.send(404, "text/plain", "Not found");
  }
}

void handleMkdir() {
  if (!server.hasArg("path")) {
    server.send(400, "text/plain", "Missing path");
    return;
  }
  if (SD.mkdir(server.arg("path"))) {
    server.send(200, "text/plain", "Folder created");
  } else {
    server.send(500, "text/plain", "Failed to create folder");
  }
}

void handleRename() {
  if (!server.hasArg("from") || !server.hasArg("to")) {
    server.send(400, "text/plain", "Missing arguments");
    return;
  }
  if (SD.rename(server.arg("from"), server.arg("to"))) {
    server.send(200, "text/plain", "Renamed");
  } else {
    server.send(500, "text/plain", "Rename failed");
  }
}

void handleDownload() {
  if (!server.hasArg("path")) {
    server.send(400, "text/plain", "Missing path");
    return;
  }
  String path = server.arg("path");
  File file = SD.open(path);
  if (!file || file.isDirectory()) {
    server.send(404, "text/plain", "File not found");
    return;
  }
  server.streamFile(file, "application/octet-stream");
  file.close();
}

void setup() {
  Serial.begin(115200);

  // Init SD card with HSPI
  spiHSPI.begin(14, 12, 13, SD_CS); // SCK, MISO, MOSI, SS
  if (!SD.begin(SD_CS, spiHSPI)) {
    Serial.println("SD Card failed!");
    return;
  }
  Serial.println("SD Card OK");

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\nWiFi Connected. IP:");
  Serial.println(WiFi.localIP());

  // Routes
  server.on("/", handleRoot);
  server.on("/list", handleList);
  server.on("/upload", HTTP_POST, [](){ server.send(200); }, handleUpload);
  server.on("/delete", handleDelete);
  server.on("/mkdir", handleMkdir);
  server.on("/rename", handleRename);
  server.on("/download", handleDownload);
  server.begin();

  Serial.println("HTTP Server started");
}

void loop() {
  server.handleClient();
}
