#include <WiFi.h>
#include <SPI.h>
#include <FS.h>
#include <SD.h>
#include <WebServer.h>

#define SD_CS 5
#define SD_CLK 14
#define SD_MISO 2
#define SD_MOSI 15

WebServer server(80);

const char* ssid = "AminJoon";
const char* password = "123456789";
File fsUploadFile;

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="fa">
<head>
  <meta charset="UTF-8">
  <title>مدیریت فایل ESP32</title>
  <style>
    body { font-family: sans-serif; direction: rtl; background: #f0f0f0; margin: 0; padding: 20px; }
    h1 { color: #333; }
    table { border-collapse: collapse; width: 100%; background: #fff; margin-top: 10px; }
    th, td { padding: 10px; border: 1px solid #ddd; text-align: right; }
    tr:hover { background-color: #f9f9f9; }
    a { text-decoration: none; color: blue; }
    input, button { padding: 6px 10px; margin-top: 10px; }
  </style>
</head>
<body>
  <h1>📂 فایل‌های SD کارت</h1>
  <form method="POST" action="/upload" enctype="multipart/form-data">
    <input type="file" name="upload"><input type="submit" value="آپلود فایل">
  </form>
  <form method="POST" action="/mkdir">
    <input type="text" name="name" placeholder="نام پوشه جدید"><input type="submit" value="ساخت پوشه">
  </form>
  <div id="list">
    %FILE_LIST%
  </div>
</body>
</html>
)rawliteral";

String formatBytes(size_t bytes) {
  if (bytes < 1024) return String(bytes) + " B";
  else if (bytes < (1024 * 1024)) return String(bytes / 1024.0, 1) + " KB";
  else return String(bytes / 1024.0 / 1024.0, 1) + " MB";
}

String listDir(File dir, String path = "") {
  String output = "<table><tr><th>نام</th><th>حجم</th><th>عملیات</th></tr>";
  while (true) {
    File entry = dir.openNextFile();
    if (!entry) break;
    String name = String(entry.name());
    bool isDir = entry.isDirectory();
    output += "<tr>";
    output += "<td><a href=\"" + name + "\">" + name + "</a></td>";
    output += "<td>" + (isDir ? "-" : formatBytes(entry.size())) + "</td>";
    output += "<td><a href=\"/delete?name=" + name + "\">❌ حذف</a></td>";
    output += "</tr>";
    entry.close();
  }
  output += "</table>";
  return output;
}



void handleRoot() {
  File root = SD.open("/");
  String html = INDEX_HTML;
  html.replace("%FILE_LIST%", listDir(root));
  root.close();
  server.send(200, "text/html", html);
}

void handleFileDownload() {
  String name = server.uri();
  File file = SD.open(name);
  if (!file || file.isDirectory()) {
    server.send(404, "text/plain", "File not found");
    return;
  }
  server.sendHeader("Content-Type", "application/octet-stream");
  server.sendHeader("Content-Disposition", "attachment; filename=\"" + name + "\"");
  server.streamFile(file, "application/octet-stream");
  file.close();
}

void handleDelete() {
  String name = server.arg("name");
  if (SD.exists(name)) {
    File f = SD.open(name);
    if (f.isDirectory()) SD.rmdir(name.c_str());
    else SD.remove(name.c_str());
    f.close();
  }
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleUpload() {
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    String filename = "/" + upload.filename;
    fsUploadFile = SD.open(filename, FILE_WRITE);
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (fsUploadFile) fsUploadFile.write(upload.buf, upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_END) {
    if (fsUploadFile) fsUploadFile.close();
  }
}

void handleMkdir() {
  String folder = server.arg("name");
  if (!folder.startsWith("/")) folder = "/" + folder;
  SD.mkdir(folder);
  server.sendHeader("Location", "/");
  server.send(303);
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nWiFi connected. IP: " + WiFi.localIP().toString());

  SPI.begin(SD_CLK, SD_MISO, SD_MOSI, SD_CS);
  if (!SD.begin(SD_CS)) {
    Serial.println("SD mount failed!");
    return;
  }
  Serial.println("SD mounted.");

  server.on("/", HTTP_GET, handleRoot);
  server.on("/delete", HTTP_GET, handleDelete);
  server.on("/mkdir", HTTP_POST, handleMkdir);
  server.on("/upload", HTTP_POST, []() {
    server.sendHeader("Location", "/");
    server.send(303);
  }, handleUpload);
  server.onNotFound(handleFileDownload);

  server.begin();
  Serial.println("HTTP server started.");
}

void loop() {
  server.handleClient();
}

