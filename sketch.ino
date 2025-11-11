#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Servo.h>

// === OLED setup ===
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SDA_PIN 9
#define SCL_PIN 8
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// === Pin Setup ===
#define LED1 2
#define BUZZER 40
#define BUTTON1 39
#define POT_PIN 6
#define ENCODER_CLK 11
#define ENCODER_DT 12
#define ENCODER_SW 13
#define SERVO_PIN 3
#define DIR_PIN 17
#define STEP_PIN 16
#define ENABLE_PIN 15

Servo myServo;

// === Variabel Global ===
volatile int encoderPos = 0;
volatile int potValue = 0;
volatile int servoAngle = 0;

// === Task Handles ===
TaskHandle_t TaskLEDHandle;
TaskHandle_t TaskBuzzerHandle;
TaskHandle_t TaskButtonHandle;
TaskHandle_t TaskOLEDHandle;
TaskHandle_t TaskServoHandle;
TaskHandle_t TaskStepperHandle;
TaskHandle_t TaskEncoderHandle;
TaskHandle_t TaskPotHandle;

// === Task ===
void TaskLED(void *pvParameters) {
  pinMode(LED1, OUTPUT);
  while (1) {
    digitalWrite(LED1, HIGH);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    digitalWrite(LED1, LOW);
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

void TaskBuzzer(void *pvParameters) {
  pinMode(BUZZER, OUTPUT);
  while (1) {
    digitalWrite(BUZZER, HIGH);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    digitalWrite(BUZZER, LOW);
    vTaskDelay(900 / portTICK_PERIOD_MS);
  }
}

void TaskButton(void *pvParameters) {
  pinMode(BUTTON1, INPUT_PULLUP);
  while (1) {
    if (digitalRead(BUTTON1) == LOW) {
      Serial.println("Button ditekan!");
      digitalWrite(BUZZER, HIGH);
      vTaskDelay(100 / portTICK_PERIOD_MS);
      digitalWrite(BUZZER, LOW);
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

void TaskPot(void *pvParameters) {
  analogReadResolution(12);
  analogSetPinAttenuation(POT_PIN, ADC_11db);
  int lastVal = 0;
  unsigned long lastPrint = 0;

  while (1) {
    int raw = analogRead(POT_PIN);
    // gunakan range kalibrasi agar sesuai
    int scaled = map(raw, 180, 3660, 0, 180);
    scaled = constrain(scaled, 0, 180);

    // jika berubah signifikan, simpan
    if (abs(scaled - lastVal) > 2) {
      potValue = scaled;
      lastVal = scaled;
    }

    // cetak ke Serial setiap 1 detik
    if (millis() - lastPrint > 1000) {
      Serial.print("Potensio: ");
      Serial.print(raw);
      Serial.print(" (mapped: ");
      Serial.print(potValue);
      Serial.println("°)");
      lastPrint = millis();
    }

    vTaskDelay(200 / portTICK_PERIOD_MS);
  }
}

void TaskEncoder(void *pvParameters) {
  pinMode(ENCODER_CLK, INPUT);
  pinMode(ENCODER_DT, INPUT);
  int lastState = digitalRead(ENCODER_CLK);
  while (1) {
    int newState = digitalRead(ENCODER_CLK);
    if (newState != lastState) {
      if (digitalRead(ENCODER_DT) != newState) encoderPos++;
      else encoderPos--;
    }
    lastState = newState;
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void TaskServo(void *pvParameters) {
  myServo.setPeriodHertz(50);
  myServo.attach(SERVO_PIN, 500, 2400);

  int testAngles[4] = {30, 90, 150, 90};
  int currentIndex = 0;
  int currentAngle = 90;
  unsigned long lastMove = 0;

  while (1) {
    // Ganti posisi setiap 3 detik
    if (millis() - lastMove > 3000) {
      int target = testAngles[currentIndex];
      Serial.print("Servo menuju: ");
      Serial.println(target);

      while (currentAngle != target) {
        if (target > currentAngle) currentAngle++;
        else currentAngle--;
        myServo.write(currentAngle);
        servoAngle = currentAngle;
        vTaskDelay(30 / portTICK_PERIOD_MS);
      }

      currentIndex++;
      if (currentIndex >= 4) currentIndex = 0;
      lastMove = millis();
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void TaskStepper(void *pvParameters) {
  pinMode(DIR_PIN, OUTPUT);
  pinMode(STEP_PIN, OUTPUT);
  pinMode(ENABLE_PIN, OUTPUT);
  digitalWrite(ENABLE_PIN, LOW);
  unsigned long lastStep = 0;

  while (1) {
    if (millis() - lastStep > 3000) {
      digitalWrite(DIR_PIN, HIGH);
      for (int i = 0; i < 100; i++) {
        digitalWrite(STEP_PIN, HIGH);
        delayMicroseconds(1200);
        digitalWrite(STEP_PIN, LOW);
        delayMicroseconds(1200);
      }
      digitalWrite(DIR_PIN, LOW);
      for (int i = 0; i < 100; i++) {
        digitalWrite(STEP_PIN, HIGH);
        delayMicroseconds(1200);
        digitalWrite(STEP_PIN, LOW);
        delayMicroseconds(1200);
      }
      lastStep = millis();
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void TaskOLED(void *pvParameters) {
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED gagal diinisialisasi");
    vTaskDelete(NULL);
  }

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  unsigned long lastUpdate = 0;

  while (1) {
    if (millis() - lastUpdate > 1000) {  // Update tiap 1 detik
      display.clearDisplay();
      display.setCursor(0, 0);
      display.print("Pot: ");
      display.println(potValue);
      display.print("Servo: ");
      display.println(servoAngle);
      display.print("Encoder: ");
      display.println(encoderPos);
      display.display();

      // cetak ke serial juga, tapi jarang
      Serial.print("OLED Update -> Pot=");
      Serial.print(potValue);
      Serial.print(" | Servo=");
      Serial.print(servoAngle);
      Serial.print(" | Encoder=");
      Serial.println(encoderPos);

      lastUpdate = millis();
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);

  xTaskCreatePinnedToCore(TaskLED, "TaskLED", 2048, NULL, 1, &TaskLEDHandle, 0);
  xTaskCreatePinnedToCore(TaskBuzzer, "TaskBuzzer", 2048, NULL, 1, &TaskBuzzerHandle, 0);
  xTaskCreatePinnedToCore(TaskButton, "TaskButton", 2048, NULL, 2, &TaskButtonHandle, 0);
  xTaskCreatePinnedToCore(TaskPot, "TaskPot", 2048, NULL, 3, &TaskPotHandle, 0);
  xTaskCreatePinnedToCore(TaskEncoder, "TaskEncoder", 2048, NULL, 3, &TaskEncoderHandle, 0);
  xTaskCreatePinnedToCore(TaskServo, "TaskServo", 4096, NULL, 2, &TaskServoHandle, 1);
  xTaskCreatePinnedToCore(TaskStepper, "TaskStepper", 4096, NULL, 1, &TaskStepperHandle, 1);
  xTaskCreatePinnedToCore(TaskOLED, "TaskOLED", 4096, NULL, 4, &TaskOLEDHandle, 1);
}

void loop() {
  // Semua task dijalankan oleh FreeRTOS
}
