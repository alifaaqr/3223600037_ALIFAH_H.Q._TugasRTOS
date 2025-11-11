// Pin definitions
#define BTN1_PIN 4
#define BTN2_PIN 5
#define LED1_PIN 2
#define LED2_PIN 15

// Task handles
TaskHandle_t Task1Handle;
TaskHandle_t Task2Handle;

// Button states
volatile bool btn1State = false;
volatile bool btn2State = false;

// LED states
volatile bool led1State = false;
volatile bool led2State = false;

// Semaphore for serial output protection
SemaphoreHandle_t serialMutex;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // Create mutex before any tasks start
  serialMutex = xSemaphoreCreateMutex();
  
  Serial.println("ESP32-S3 Dual Core Button Control");

  // Initialize pins
  pinMode(BTN1_PIN, INPUT_PULLUP);
  pinMode(BTN2_PIN, INPUT_PULLUP);
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  
  // Create Task 1 on Core 0
  xTaskCreatePinnedToCore(
    Task1Code,      // Function
    "Task1",        // Name
    10000,          // Stack size
    NULL,           // Parameters
    1,              // Priority
    &Task1Handle,   // Task handle
    0               // Core 0
  );
  
  // Create Task 2 on Core 1
  xTaskCreatePinnedToCore(
    Task2Code,      // Function
    "Task2",        // Name
    10000,          // Stack size
    NULL,           // Parameters
    1,              // Priority
    &Task2Handle,   // Task handle
    1               // Core 1
  );
  
  // Use mutex for setup prints too since tasks might run immediately
  if (xSemaphoreTake(serialMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    Serial.println("Tasks created successfully!");
    Serial.println("BTN1 controls LED1 on Core 0");
    Serial.println("BTN2 controls LED2 on Core 1\n");
    xSemaphoreGive(serialMutex);
  }
}

// Task 1: Running on Core 0
void Task1Code(void * parameter) {
  if (xSemaphoreTake(serialMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    Serial.printf("Task1 running on core %d\n", xPortGetCoreID());
    xSemaphoreGive(serialMutex);
  }
  
  bool lastBtn1State = true;
  unsigned long lastDebounceTime = 0;
  const unsigned long debounceDelay = 50;
  
  for(;;) {
    // Read button with debouncing
    bool reading = digitalRead(BTN1_PIN);
    
    if (reading != lastBtn1State) {
      lastDebounceTime = millis();
    }
    
    if ((millis() - lastDebounceTime) > debounceDelay) {
      if (reading != btn1State) {
        btn1State = reading;
        
        // Button pressed (LOW because of pullup)
        if (btn1State == LOW) {
          led1State = !led1State;
          digitalWrite(LED1_PIN, led1State);
          
          // Protected serial output
          if (xSemaphoreTake(serialMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            Serial.printf("[CORE 0] BTN1 pressed! LED1: %s\n", led1State ? "ON" : "OFF");
            xSemaphoreGive(serialMutex);
          }
        }
      }
    }
    
    lastBtn1State = reading;
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

// Task 2: Running on Core 1
void Task2Code(void * parameter) {
  if (xSemaphoreTake(serialMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    Serial.printf("Task2 running on core %d\n", xPortGetCoreID());
    xSemaphoreGive(serialMutex);
  }
  
  bool lastBtn2State = true;
  unsigned long lastDebounceTime = 0;
  const unsigned long debounceDelay = 50;
  
  for(;;) {
    // Read button with debouncing
    bool reading = digitalRead(BTN2_PIN);
    
    if (reading != lastBtn2State) {
      lastDebounceTime = millis();
    }
    
    if ((millis() - lastDebounceTime) > debounceDelay) {
      if (reading != btn2State) {
        btn2State = reading;
        
        // Button pressed (LOW because of pullup)
        if (btn2State == LOW) {
          led2State = !led2State;
          digitalWrite(LED2_PIN, led2State);
          
          // Protected serial output
          if (xSemaphoreTake(serialMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            Serial.printf("[CORE 1] BTN2 pressed! LED2: %s\n", led2State ? "ON" : "OFF");
            xSemaphoreGive(serialMutex);
          }
        }
      }
    }
    
    lastBtn2State = reading;
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void loop() {
  // Not used
}