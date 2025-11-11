#include <Arduino.h>

// === Konfigurasi Pin A4988 sesuai .json ===
#define DIR_PIN     18   // A4988 -> DIR
#define STEP_PIN    19   // A4988 -> STEP
#define ENABLE_PIN  21   // A4988 -> ENABLE

// === Variabel global ===
volatile int stepDelay = 1000;  // mikrodetik antar langkah
volatile bool motorOn = true;   // status motor

// === Task untuk Core 0 (Kontrol motor) ===
void TaskMotorControl(void *pvParameters) {
  pinMode(DIR_PIN, OUTPUT);
  pinMode(STEP_PIN, OUTPUT);
  pinMode(ENABLE_PIN, OUTPUT);
  
  digitalWrite(ENABLE_PIN, LOW);   // aktifkan driver (aktif LOW)
  digitalWrite(DIR_PIN, HIGH);     // arah awal ke kanan

  Serial.println("Core 0: Kontrol motor aktif");

  while (true) {
    if (motorOn) {
      digitalWrite(STEP_PIN, HIGH);
      delayMicroseconds(stepDelay);
      digitalWrite(STEP_PIN, LOW);
      delayMicroseconds(stepDelay);
    } else {
      vTaskDelay(10 / portTICK_PERIOD_MS);
    }
  }
}

// === Task untuk Core 1 (Pemantauan Serial) ===
void TaskMonitor(void *pvParameters) {
  Serial.println("Core 1: Pemantauan dimulai...");
  while (true) {
    Serial.printf("[Core %d] Motor %s | Delay: %d us\n",
                  xPortGetCoreID(),
                  motorOn ? "ON" : "OFF",
                  stepDelay);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

// === Setup ===
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== ESP32-S3 Dual Core Stepper Test (Wokwi) ===");

  // Task untuk Core 0: kontrol stepper
  xTaskCreatePinnedToCore(
    TaskMotorControl,
    "MotorControl",
    4096,
    NULL,
    1,
    NULL,
    0 // Core 0
  );

  // Task untuk Core 1: monitoring serial
  xTaskCreatePinnedToCore(
    TaskMonitor,
    "Monitor",
    4096,
    NULL,
    1,
    NULL,
    1 // Core 1
  );
}

// === Loop utama ===
void loop() {
  static unsigned long lastToggle = 0;
  if (millis() - lastToggle > 5000) {
    digitalWrite(DIR_PIN, !digitalRead(DIR_PIN)); // ganti arah
    Serial.println("Arah motor diubah!");
    lastToggle = millis();
  }
}
