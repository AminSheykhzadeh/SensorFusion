/*

SCK → GPIO 14
MISO → GPIO 12
MOSI → GPIO 13
CS → GPIO 5 (قابل تغییر)

*/

#include <SPI.h>
#include <SD.h>

#define SD_CS 5  // پین CS مورد استفاده برای SD کارت

// تعریف یک SPIClass جدید برای HSPI
SPIClass hspi(HSPI);

void setup() {
  Serial.begin(115200);
  delay(500);

  // راه‌اندازی HSPI با پین‌های دلخواه
  hspi.begin(14, 12, 13, SD_CS);  // SCK, MISO, MOSI, CS

  // شروع ارتباط با SD کارت با HSPI
  if (!SD.begin(SD_CS, hspi)) {
    Serial.println("❌ SD Card initialization failed!");
    return;
  }
  Serial.println("✅ SD Card initialized on HSPI.");

  // باز کردن ریشه SD کارت
  File root = SD.open("/");
  if (!root) {
    Serial.println("❌ Failed to open root directory.");
    return;
  }

  // لیست کردن محتوای SD کارت
  Serial.println("📁 SD Card Contents:");
  printDirectory(root, 0);
  root.close();
}

void loop() {
  // در این مثال نیازی به کاری در loop نیست
}

// تابع بازگشتی برای چاپ فایل‌ها و پوشه‌ها
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
      Serial.print(entry.size());
      Serial.println(" bytes");
    }
    entry.close();
  }
}
