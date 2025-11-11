#include <Arduino.h>
#include <ESP32Servo.h>
#include <Wire.h>            
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "freertos/task.h" 


// PIN I2C (OLED)
#define OLED_SDA 37
#define OLED_SCL 38
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1   


// PIN KONTROL
const int buzzerPin = 6;      
const int servoPin = 14;       
const int stepperEnablePin = 17;


// PIN INPUT
const int potPin = 4;         
const int button1Pin = 3;     
const int button2Pin = 9;     
const int rotaryClk = 16;      
const int rotaryDt = 15;       
const int rotarySW = 39;       


// PIN LED
const int led1Pin = 10;
const int led2Pin = 11;
const int led3Pin = 5; 


// PIN STEPPER
const int dirPin = 12; 
const int stepPin = 13;


Servo myservo;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);








// Variabel Global Shared
// volatile digunakan karena dimodifikasi oleh Core 0 dan dibaca oleh Core 1
volatile int currentMode = 0;
long lastStepTime = 0;       
int defaultStepperDelay = 500;


volatile int potValue = 0;            
int servoSweepDelay = 15;    


// Variabel Debouncing & Encoder
volatile int encoderCount = 0;
static unsigned long lastInteractionTime = 0;
#define DEBOUNCE_DELAY 50




// --- FUNGSI UTILITAS ---


void updateDisplay(String mode, String data) {
 display.clearDisplay();
 display.setTextSize(1);
 display.setTextColor(SSD1306_WHITE);
 display.setCursor(0, 0);
 display.println("S3 UNIVERSAL CONTROLLER");
 display.drawFastHLine(0, 9, 128, SSD1306_WHITE);


 display.setTextSize(2);
 display.setCursor(0, 15);
 display.print("MODE: ");
 display.println(mode);


 display.setTextSize(1);
 display.setCursor(0, 35);
 display.println(data);
 display.display();
}


void IRAM_ATTR readEncoder() {
 if (millis() - lastInteractionTime > 1) {
   if (digitalRead(rotaryDt) != digitalRead(rotaryClk)) {
     encoderCount++;
   } else {
     encoderCount--;
   }
   lastInteractionTime = millis();
 }
}


// Task yang berjalan di Core 0 untuk pembacaan Potensiometer
void TaskCore0(void * parameter) {
   static long lastCore0LogTime = 0;
   const long logInterval = 500; // Log setiap 500ms


   for (;;) {
       // Pembacaan Potensiometer dilakukan di Core 0
       potValue = analogRead(potPin);


       // LOGGING UNTUK BUKTI CORE 0
       if (millis() - lastCore0LogTime >= logInterval) {
           Serial.print("[CORE ");
           Serial.print(xPortGetCoreID()); // Output seharusnya 0
           Serial.print("] PotValue: ");
           Serial.println(potValue);
           lastCore0LogTime = millis();
       }


       // Memberikan jeda waktu agar FreeRTOS dapat menjadwalkan
       vTaskDelay(5 / portTICK_PERIOD_MS);
   }
}




// --- SETUP ---


void setup() {
 Serial.begin(115200);
  // Log di mana setup berjalan (biasanya Core 1)
 Serial.print("SETUP berjalan di CORE: ");
 Serial.println(xPortGetCoreID());


 // --- 1. Konfigurasi OLED ---
 Wire.begin(OLED_SDA, OLED_SCL);
 if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
   Serial.println(F("SSD1306 alokasi gagal"));
   for(;;);
 }
 display.display();
 delay(1000);
 updateDisplay("IDLE", "Tekan Tombol 1 utk Mulai");


 // --- 2. Konfigurasi Pin I/O ---
 pinMode(stepperEnablePin, OUTPUT);
 pinMode(dirPin, OUTPUT);
 pinMode(stepPin, OUTPUT);
 pinMode(buzzerPin, OUTPUT);
 pinMode(led1Pin, OUTPUT);
 pinMode(led2Pin, OUTPUT);
 pinMode(led3Pin, OUTPUT);
  // Semua Input digital menggunakan INPUT_PULLUP
 pinMode(button1Pin, INPUT_PULLUP);
 pinMode(button2Pin, INPUT_PULLUP);
 pinMode(rotarySW, INPUT_PULLUP);
 pinMode(rotaryClk, INPUT_PULLUP);
 pinMode(rotaryDt, INPUT_PULLUP); 


 // Konfigurasi Pin Potensiometer
 pinMode(potPin, INPUT);


 // Konfigurasi ADC untuk Potensiometer
 analogReadResolution(12); // Resolusi 12 bit (nilai 0 hingga 4095)
 analogSetAttenuation(ADC_11db); // Rentang tegangan 0V-3.3V


 // --- 3. Konfigurasi Rotary Encoder ---
 attachInterrupt(digitalPinToInterrupt(rotaryClk), readEncoder, FALLING);


 // --- 4. Konfigurasi Servo ---
 myservo.attach(servoPin);
 myservo.write(0);


 // --- 5. Status Awal Sistem ---
 digitalWrite(stepperEnablePin, LOW);
 digitalWrite(led1Pin, LOW);
 digitalWrite(led2Pin, LOW);
 digitalWrite(led3Pin, LOW);
 digitalWrite(buzzerPin, LOW);
  Serial.println("System Ready. Dual Core running. Check Serial Monitor for Core ID proof.");


 // Inisiasi Task Core 0 untuk pembacaan Potensiometer
 xTaskCreatePinnedToCore(
   TaskCore0,   // Task function
   "InputPotTask",// Task name
   4096,        // Stack size
   NULL,        // Parameter
   1,           // Priority
   NULL,        // Task handle
   0);          // Core ID (Core 0)
}




// --- LOOP (CORE 1) ---
// Loop ini sekarang hanya fokus pada Logika dan Output
void loop() {
 static long lastCore1LogTime = 0;
 const long logInterval = 500; // Log setiap 500ms


 // LOGGING UNTUK BUKTI CORE 1
 if (millis() - lastCore1LogTime >= logInterval) {
     Serial.print("[CORE ");
     Serial.print(xPortGetCoreID()); // Output seharusnya 1
     Serial.println("] Loop utama (Output/Logic) aktif.");
     lastCore1LogTime = millis();
 }
  int stateButton1 = digitalRead(button1Pin);
 int stateButton2 = digitalRead(button2Pin);
  // === A. KONTROL MODE (Tombol 1) ===
 if (stateButton1 == LOW) {
   if (millis() - lastInteractionTime > DEBOUNCE_DELAY) {
     currentMode = (currentMode % 3) + 1;
     digitalWrite(buzzerPin, HIGH); delay(50); digitalWrite(buzzerPin, LOW);
     lastInteractionTime = millis();
   }
 }


 // === B. EKSESEKUSI MODE ===
 if (currentMode == 1) {
   // MODE 1: STEPPER MOTOR
  
   // LED 1 Blink (Indikasi Mode 1)
   digitalWrite(led1Pin, !digitalRead(led1Pin));
   digitalWrite(led2Pin, LOW);
   digitalWrite(led3Pin, LOW);
  
   // Atur arah putaran Stepper (diperlukan agar motor berjalan)
   digitalWrite(dirPin, HIGH);
  
   // Kontrol Kecepatan Stepper dengan Potensiometer
   // Potensiometer mengatur delay step (500 us cepat, 5000 us lambat)
   int stepperDelayControl = map(potValue, 0, 4095, 500, 5000);


   // Gerakan Stepper
   if (millis() - lastStepTime > stepperDelayControl) { // Gunakan delay yang dikontrol Pot
     digitalWrite(stepPin, !digitalRead(stepPin));
     lastStepTime = millis();
   }
  
   String data = "Delay Kontrol Pot: " + String(stepperDelayControl) + " us\nNilai Pot: " + String(potValue);
   updateDisplay("STEPPER", data);
  
 } else if (currentMode == 2) {
   // MODE 2: SERVO SWEEP (Potensiometer Kontrol Kecepatan)
  
   // LED 2 Blink (Indikasi Mode 2)
   digitalWrite(led2Pin, !digitalRead(led2Pin));
   digitalWrite(led1Pin, LOW);
   digitalWrite(led3Pin, LOW);
  
   // MAPPING POTENSIOMETER
   // 0 (Kiri) -> 20ms (Cepat); 4095 (Kanan) -> 200ms (Lambat)
   servoSweepDelay = map(potValue, 0, 4095, 20, 200);  
   static int angle = 0;
   static int direction = 1;
  
   myservo.write(angle);
   // Langkah sudut dipercepat
   angle += (direction * 2);


   if (angle >= 180) direction = -1;
   if (angle <= 0) direction = 1;


   digitalWrite(buzzerPin, HIGH); delay(2); digitalWrite(buzzerPin, LOW);


   String data = "Delay: " + String(servoSweepDelay) + " ms\nSudut Servo: " + String(angle) + " deg";
   updateDisplay("SERVO SWEEP", data);
   delay(servoSweepDelay); // Kecepatan diatur Potensiometer
  
 } else if (currentMode == 3) {
   // MODE 3: ROTARY ENCODER TEST
  
   // LED 3 Blink (Indikasi Mode 3)
   digitalWrite(led3Pin, !digitalRead(led3Pin));
   digitalWrite(led1Pin, LOW);
   digitalWrite(led2Pin, LOW);
  
   // Tombol Rotary Switch Reset Nilai
   if (digitalRead(rotarySW) == LOW) {
     if (millis() - lastInteractionTime > DEBOUNCE_DELAY) {
       encoderCount = 0;
       digitalWrite(buzzerPin, HIGH); delay(100); digitalWrite(buzzerPin, LOW);
       lastInteractionTime = millis();
     }
   }
  
   String data = "Putar = Ubah Nilai\nTekan = Reset Nilai";
   updateDisplay("ENCODER", "Nilai: " + String(encoderCount));
   delay(50);
  
 } else {
   // MODE 0 (IDLE) - Semua Komponen Kontrol Mati
  
   // Matikan semua indikator
   digitalWrite(led1Pin, LOW);
   digitalWrite(led2Pin, LOW);
   digitalWrite(led3Pin, LOW);
   digitalWrite(buzzerPin, LOW);


   updateDisplay("IDLE", "Tekan Tombol 1 utk Mulai");
 }


 // === C. KONTROL RESET/TENGAH (Tombol 2) ===
 if (stateButton2 == LOW) {
   if (millis() - lastInteractionTime > DEBOUNCE_DELAY) {
     digitalWrite(buzzerPin, HIGH); delay(200); digitalWrite(buzzerPin, LOW);
    
     currentMode = 0; // Pindah ke Idle
     myservo.write(0);
     encoderCount = 0;
    
     updateDisplay("IDLE", "RESET: Tekan Tombol 1 utk Mulai");
     lastInteractionTime = millis();
   }
 }


 delay(1);
}
