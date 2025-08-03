#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

// ==== OLED Setup ====
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ==== Setup ====
void setup() {
  Serial.begin(9600);
  pinMode(5, OUTPUT);

  // OLED init
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 allocation failed");
    while (true);
  }
  display.clearDisplay();
  display.display();
}

// ==== Globals ====
bool aboveThreshold = false;
unsigned long lastTriggerTime = 0;
unsigned long timeBetweenTriggers = 0;
unsigned long speed = 0;

// ==== Read analog voltage ====
float readVoltage(int pin) {
  analogRead(pin);                 // throw first reading away
  delayMicroseconds(5);           // let ADC settle
  int analogValue = analogRead(pin);
  return analogValue * (5.0 / 1023.0);
}

// ==== Main Loop ====
void loop() {
  float voltage = readVoltage(A0);
  handleThresholdCrossing(voltage, 3.0);
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
