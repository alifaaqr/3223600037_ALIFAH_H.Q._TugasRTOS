#include <Arduino.h>

// PIN SESUAI WIRING WOKWI
#define BUZZER1_PIN 18  // Buzzer kiri (Core 0)
#define BUZZER2_PIN 5   // Buzzer kanan (Core 1)

// Frekuensi nada (dalam Hz)
#define FREQ_BUZZER1 500  // 800 Hz untuk buzzer 1
#define FREQ_BUZZER2 1500  // 1.5 kHz untuk buzzer 2

// Semaphore untuk sinkronisasi
SemaphoreHandle_t buzzer1Semaphore;
SemaphoreHandle_t buzzer2Semaphore;

// Task untuk Core 0 (PRO_CPU) - Buzzer 1
void buzzerCore0Task(void *pvParameters) {
  ledcAttach(BUZZER1_PIN, FREQ_BUZZER1, 8);
  
  while (1) {
    // Tunggu giliran buzzer 1
    xSemaphoreTake(buzzer1Semaphore, portMAX_DELAY);
    
    ledcWrite(BUZZER1_PIN, 128);
    Serial.printf("[Core %d] Buzzer 1 ON (500 Hz)\n", xPortGetCoreID());
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    
    ledcWrite(BUZZER1_PIN, 0);
    Serial.printf("[Core %d] Buzzer 1 OFF\n", xPortGetCoreID());
    vTaskDelay(500 / portTICK_PERIOD_MS);
    
    // Beri giliran ke buzzer 2
    xSemaphoreGive(buzzer2Semaphore);
  }
}

// Task untuk Core 1 (APP_CPU) - Buzzer 2
void buzzerCore1Task(void *pvParameters) {
  ledcAttach(BUZZER2_PIN, FREQ_BUZZER2, 8);
  
  while (1) {
    // Tunggu giliran buzzer 2
    xSemaphoreTake(buzzer2Semaphore, portMAX_DELAY);
    
    ledcWrite(BUZZER2_PIN, 128);
    Serial.printf("[Core %d] Buzzer 2 ON (1500 Hz)\n", xPortGetCoreID());
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    
    ledcWrite(BUZZER2_PIN, 0);
    Serial.printf("[Core %d] Buzzer 2 OFF\n", xPortGetCoreID());
    vTaskDelay(500 / portTICK_PERIOD_MS);
    
    // Beri giliran ke buzzer 1
    xSemaphoreGive(buzzer1Semaphore);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("ESP32-S3 Dual-Core Buzzer Control - ALTERNATING");

  // Buat semaphore
  buzzer1Semaphore = xSemaphoreCreateBinary();
  buzzer2Semaphore = xSemaphoreCreateBinary();

  // Buat task di Core 0 untuk Buzzer 1
  xTaskCreatePinnedToCore(
    buzzerCore0Task, 
    "Buzzer1_Core0", 
    2048, 
    NULL, 
    1, 
    NULL, 
    0
  );

  // Buat task di Core 1 untuk Buzzer 2
  xTaskCreatePinnedToCore(
    buzzerCore1Task, 
    "Buzzer2_Core1", 
    2048, 
    NULL, 
    1, 
    NULL, 
    1
  );
  
  Serial.println("Both buzzer tasks created!");
  
  // Mulai dengan buzzer 1
  xSemaphoreGive(buzzer1Semaphore);
}

void loop() {
  delay(1000);
}