#include <Arduino.h>

// SESUAIKAN PIN DENGAN WIRING WOKWI!
#define LED1_PIN 18  // Di Wokwi: led1 terhubung ke GPIO18
#define LED2_PIN 17  // Di Wokwi: led2 terhubung ke GPIO17
#define LED3_PIN 5   // Di Wokwi: led3 terhubung ke GPIO5

// Task untuk Core 0 (PRO_CPU)
void ledCore0Task1(void *pvParameters) {
  pinMode(LED1_PIN, OUTPUT);
  while (1) {
    digitalWrite(LED1_PIN, HIGH);
    Serial.printf("LED1 ON | Core: %d\n", xPortGetCoreID());
    vTaskDelay(700 / portTICK_PERIOD_MS);
    digitalWrite(LED1_PIN, LOW);
    Serial.printf("LED1 OFF | Core: %d\n", xPortGetCoreID());
    vTaskDelay(700 / portTICK_PERIOD_MS);
  }
}

void ledCore0Task2(void *pvParameters) {
  pinMode(LED2_PIN, OUTPUT);
  while (1) {
    digitalWrite(LED2_PIN, HIGH);
    Serial.printf("LED2 ON | Core: %d\n", xPortGetCoreID());
    vTaskDelay(400 / portTICK_PERIOD_MS);
    digitalWrite(LED2_PIN, LOW);
    Serial.printf("LED2 OFF | Core: %d\n", xPortGetCoreID());
    vTaskDelay(400 / portTICK_PERIOD_MS);
  }
}

// Task untuk Core 1 (APP_CPU)
void ledCore1Task(void *pvParameters) {
  pinMode(LED3_PIN, OUTPUT);
  while (1) {
    digitalWrite(LED3_PIN, HIGH);
    Serial.printf("LED3 ON | Core: %d\n", xPortGetCoreID());
    vTaskDelay(200 / portTICK_PERIOD_MS);
    digitalWrite(LED3_PIN, LOW);
    Serial.printf("LED3 OFF | Core: %d\n", xPortGetCoreID());
    vTaskDelay(200 / portTICK_PERIOD_MS);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000); // Memberi waktu Serial Monitor siap
  Serial.println("ESP32-S3 Dual-Core LED Control");

  // Core 0: LED1 (GPIO18) dan LED2 (GPIO17)
  xTaskCreatePinnedToCore(ledCore0Task1, "LED1_Core0", 2048, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(ledCore0Task2, "LED2_Core0", 2048, NULL, 1, NULL, 0);

  // Core 1: LED3 (GPIO5)
  xTaskCreatePinnedToCore(ledCore1Task, "LED3_Core1", 2048, NULL, 1, NULL, 1);
}

void loop() {
  // Loop kosong karena semua pekerjaan ditangani oleh FreeRTOS tasks
  delay(1000);
}