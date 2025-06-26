#include <SPI.h>
#include <SD.h>

#define SD_CS_PIN 5  // 🔁 Change this to your actual CS pin (commonly 5 or 4 for ESP32)

void setup() {
  Serial.begin(115200);
  while (!Serial) ; // Wait for Serial monitor (if required)

  Serial.println("Initializing SD card...");

  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("❌ SD Card initialization failed!");
    return;
  }

  Serial.println("✅ SD Card initialized successfully!");

  // List files (optional)
  File root = SD.open("/");
  printDirectory(root, 0);
  root.close();
}

void loop() {
  // Nothing here
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
