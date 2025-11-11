#include <Arduino.h>

#define SERVO1_PIN 18
#define SERVO2_PIN 19

TaskHandle_t Task1, Task2;

// Konversi sudut (0-180) ke nilai PWM (0-4095 untuk 12-bit)
int angleToPWM(int angle) {
  int pulseUs = map(angle, 0, 180, 500, 2400);        // 500–2400 µs
  return map(pulseUs, 0, 20000, 0, 4095);              // 20000µs = periode 50Hz
}

void servoTask1(void *pv) {
  int coreID = xPortGetCoreID();
  Serial.printf("Servo1 task started on Core %d\n", coreID);
  
  while (1) {
    for (int pos = 0; pos <= 180; pos += 5) {
      analogWrite(SERVO1_PIN, angleToPWM(pos));
      if (pos % 60 == 0) Serial.printf("Core %d - Servo1: %d°\n", coreID, pos);
      vTaskDelay(20 / portTICK_PERIOD_MS);
    }
    for (int pos = 180; pos >= 0; pos -= 5) {
      analogWrite(SERVO1_PIN, angleToPWM(pos));
      if (pos % 60 == 0) Serial.printf("Core %d - Servo1: %d°\n", coreID, pos);
      vTaskDelay(20 / portTICK_PERIOD_MS);
    }
  }
}

void servoTask2(void *pv) {
  int coreID = xPortGetCoreID();
  Serial.printf("Servo2 task started on Core %d\n", coreID);
  
  while (1) {
    for (int pos = 180; pos >= 0; pos -= 5) {
      analogWrite(SERVO2_PIN, angleToPWM(pos));
      if (pos % 60 == 0) Serial.printf("Core %d - Servo2: %d°\n", coreID, pos);
      vTaskDelay(20 / portTICK_PERIOD_MS);
    }
    for (int pos = 0; pos <= 180; pos += 5) {
      analogWrite(SERVO2_PIN, angleToPWM(pos));
      if (pos % 60 == 0) Serial.printf("Core %d - Servo2: %d°\n", coreID, pos);
      vTaskDelay(20 / portTICK_PERIOD_MS);
    }
  }
}

void setup() {
  Serial.begin(115200);
  
  // ✅ Atur frekuensi dan resolusi per pin (ESP32 Core v3+)
  analogWriteResolution(SERVO1_PIN, 12);  // 12-bit = 0–4095
  analogWriteResolution(SERVO2_PIN, 12);
  
  analogWriteFrequency(SERVO1_PIN, 50);   // 50Hz untuk servo
  analogWriteFrequency(SERVO2_PIN, 50);
  
  // Posisi awal
  analogWrite(SERVO1_PIN, angleToPWM(90));
  analogWrite(SERVO2_PIN, angleToPWM(90));
  
  delay(500);
  Serial.println("✅ PWM 12-bit @ 50Hz configured. Creating dual-core tasks...");
  
  xTaskCreatePinnedToCore(servoTask1, "T1", 2048, NULL, 1, &Task1, 0);
  xTaskCreatePinnedToCore(servoTask2, "T2", 2048, NULL, 1, &Task2, 1);
  
  Serial.println("✅ Dual-core servo control ready!");
}

void loop() {
  static uint32_t last = 0;
  if (millis() - last > 5000) {
    Serial.println("[MONITOR] System running...");
    last = millis();
  }
  delay(100);
}