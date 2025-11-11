#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

// Pin I2C sesuai wiring-mu
#define SDA_PIN 20   // GPIO20 → SDA
#define SCL_PIN 19   // GPIO19 → SCL

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

SemaphoreHandle_t xMutex = NULL;

TaskHandle_t TaskOLED;
TaskHandle_t TaskSerial;

void updateOLEDTask(void *pvParameters) {
  int counter = 0;
  int coreID = xPortGetCoreID();
  
  while (1) {
    if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(0, 0);
      display.println("Dual-Core OLED Test");
      display.println("==================");
      display.printf("Core ID   : %d", coreID);
      display.printf("\nCounter   : %d", counter++);
      display.printf("\nTask      : OLED");
      display.display();
      xSemaphoreGive(xMutex);
    }
    vTaskDelay(500 / portTICK_PERIOD_MS); // Update tiap 0.5 detik
  }
}

void serialLogTask(void *pvParameters) {
  int counter = 0;
  int coreID = xPortGetCoreID();
  
  while (1) {
    // Akses OLED juga dari Core 1 (dengan mutex)
    if (xSemaphoreTake(xMutex, 10 / portTICK_PERIOD_MS) == pdTRUE) {
      display.setCursor(0, 32);
      display.printf("\nSerial Core: %d", coreID);
      display.display();
      xSemaphoreGive(xMutex);
    }
    
    Serial.printf("Core %d - Serial Log #%d\n", coreID, counter++);
    vTaskDelay(1000 / portTICK_PERIOD_MS); // Log tiap 1 detik
  }
}

void setup() {
  Serial.begin(115200);
  
  // Inisialisasi I2C dengan pin custom
  Wire.begin(SDA_PIN, SCL_PIN);
  
  // Inisialisasi OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("❌ OLED not found!");
    while (1);
  }
  
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 20);
  display.println("Starting...");
  display.display();
  
  delay(1000);
  
  // Buat mutex
  xMutex = xSemaphoreCreateMutex();
  
  // Buat task di Core 0 (OLED)
  xTaskCreatePinnedToCore(
    updateOLEDTask,
    "OLED_Task",
    4096,
    NULL,
    1,
    &TaskOLED,
    0
  );
  
  // Buat task di Core 1 (Serial)
  xTaskCreatePinnedToCore(
    serialLogTask,
    "Serial_Task",
    4096,
    NULL,
    1,
    &TaskSerial,
    1
  );
  
  Serial.println("✅ Dual-core OLED control ready!");
}

void loop() {
  // Kosong — semua dikendalikan oleh task
}