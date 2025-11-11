#include <Arduino.h>

// Pin assignments sesuai wiring di Wokwi:
const int potPin1 = 1;   // pot1 → GPIO1
const int potPin2 = 4;   // pot  → GPIO4

TaskHandle_t Task1;
TaskHandle_t Task2;

// Fungsi untuk Core 0: baca pot1 (GPIO1)
void readPot1(void *pvParameters) {
  while (1) {
    int value = analogRead(potPin1);
    Serial.printf("Core 0 - Pot (GPIO1): %d\n", value);
    delay(500);
  }
}

// Fungsi untuk Core 1: baca pot2 (GPIO4)
void readPot2(void *pvParameters) {
  while (1) {
    int value = analogRead(potPin2);
    Serial.printf("Core 1 - Pot (GPIO4): %d\n", value);
    delay(500);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(potPin1, INPUT);
  pinMode(potPin2, INPUT);

  xTaskCreatePinnedToCore(
    readPot1,
    "ReadPot1",
    2048,
    NULL,
    1,
    &Task1,
    0  // Core 0
  );

  xTaskCreatePinnedToCore(
    readPot2,
    "ReadPot2",
    2048,
    NULL,
    1,
    &Task2,
    1  // Core 1
  );
}

void loop() {
  // Tidak perlu isi — semua di-handle oleh task
}