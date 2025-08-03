#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <avr/sleep.h>
#include <avr/power.h>

// ==== OLED Setup ====
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ==== Globals ====
bool aboveThreshold = false;
unsigned long lastTriggerTime = 0;
unsigned long timeBetweenTriggers = 0;
unsigned long speed = 0;

// ==== Setup ====
void setup() {
  Serial.begin(9600);
  pinMode(5, OUTPUT);

  // OLED init
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 allocation failed");
    while (true);
  }

  display.ssd1306_command(SSD1306_SETCONTRAST);
  display.ssd1306_command(50);
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(20, 10);
  display.println("Speedometer v1.0");

  showLoadingDots(">> Initializing", 5, 30, 3, 300);

  display.setCursor(5, 45);
  display.println(">> Ready!");
  display.display();

  delay(1000);  // 2 seconds to allow uploads

  set_sleep_mode(SLEEP_MODE_IDLE);  // Set sleep mode to idle
}

// ==== Read analog voltage ====
float readVoltage(int pin) {
  analogRead(pin);                 // throw first reading away
  delayMicroseconds(5);           // let ADC settle
  int analogValue = analogRead(pin);
  return analogValue * (5.0 / 1023.0);
}

void showLoadingDots(const char* message, uint8_t x, uint8_t y, uint8_t cycles, uint16_t delayTime) {
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(x, y);
  display.println(message);
  display.display();

  for (uint8_t i = 0; i < cycles; i++) {
    for (uint8_t dots = 0; dots <= 3; dots++) {
      display.fillRect(x + (strlen(message) * 6), y, 20, 8, BLACK); // clear dot area only
      display.setCursor(x + (strlen(message) * 6), y);
      for (uint8_t j = 0; j < dots; j++) {
        display.print(".");
      }
      display.display();
      delay(delayTime);
    }
  }
}

// ==== Display speed on OLED ====
void displaySpeed(unsigned long spd) {
  display.clearDisplay();
  display.setTextSize(4);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);
  display.println(spd);
  display.setTextSize(2);
  display.print("KM/h");
  display.display();
}

// ==== Update speed based on time ====
void updateSpeed(unsigned long timeDelta) {
  if (timeDelta > 0) {
    speed = (2200*3.6) / timeDelta;
    Serial.print(speed);
    Serial.println(" KM/h");
    displaySpeed(speed);
  }
}

// ==== Handle threshold crossing ====
void handleThresholdCrossing(float voltage, float threshold) {
  if (voltage > threshold) {
    if (!aboveThreshold) {
      unsigned long currentTime = millis();
      timeBetweenTriggers = currentTime - lastTriggerTime;
      lastTriggerTime = currentTime;
      updateSpeed(timeBetweenTriggers);
      aboveThreshold = true;
    }
    digitalWrite(5, HIGH);
  } else {
    digitalWrite(5, LOW);
    aboveThreshold = false;
  }
}

// ==== Main Loop ====
void loop() {
  float voltage = readVoltage(A0);
  handleThresholdCrossing(voltage, 3.0);

  // Enter idle sleep to save power but keep timers running
  sleep_enable();
  sleep_cpu();
  sleep_disable();
}