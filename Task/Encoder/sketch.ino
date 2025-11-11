#include <Arduino.h>

// Pin encoder SESUAI WIRING DI WOKWI (GPIO16, 17, 18)
#define ENC_CLK 16
#define ENC_DT  17
#define ENC_SW  18

// Variabel global (dibaca & ditulis dari core berbeda)
volatile int encoderPos = 0;
volatile bool buttonPressed = false;
volatile bool newData = false;

// Task handles
TaskHandle_t EncoderTask;
TaskHandle_t PrintTask;

void readEncoderTask(void *pvParameters) {
  pinMode(ENC_CLK, INPUT);
  pinMode(ENC_DT, INPUT);
  pinMode(ENC_SW, INPUT_PULLUP);  // SW aktif LOW

  int lastCLK = digitalRead(ENC_CLK);

  while (1) {
    int currentCLK = digitalRead(ENC_CLK);

    if (lastCLK != currentCLK) {
      if (digitalRead(ENC_DT) != currentCLK) {
        encoderPos++; // CW
      } else {
        encoderPos--; // CCW
      }
      newData = true;
    }

    if (digitalRead(ENC_SW) == LOW) {
      buttonPressed = true;
      newData = true;
      while (digitalRead(ENC_SW) == LOW) {
        delay(10); // Debounce sederhana
      }
    }

    lastCLK = currentCLK;
    delay(1); // Polling cepat
  }
}

void printDataTask(void *pvParameters) {
  while (1) {
    if (newData) {
      int pos = encoderPos;
      bool btn = buttonPressed;
      buttonPressed = false;
      newData = false;

      if (btn) {
        Serial.printf("Core 1 - Encoder position: %d [BUTTON PRESSED!]\n", pos);
      } else {
        Serial.printf("Core 1 - Encoder position: %d\n", pos);
      }
    }
    delay(100);
  }
}

void setup() {
  Serial.begin(115200);

  // Task di Core 0: baca encoder
  xTaskCreatePinnedToCore(
    readEncoderTask,
    "EncoderTask",
    4096,
    NULL,
    2,
    &EncoderTask,
    0
  );

  // Task di Core 1: tampilkan data
  xTaskCreatePinnedToCore(
    printDataTask,
    "PrintTask",
    2048,
    NULL,
    1,
    &PrintTask,
    1
  );
}

void loop() {
  // Tidak digunakan
}