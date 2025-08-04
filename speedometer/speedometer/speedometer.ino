#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <avr/sleep.h>

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
unsigned long totalDistance_mm = 0;  // in meters

// ==== Setup ====
void setup() {
  Serial.begin(9600);

  // OLED init
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    while (true);
  }

  display.ssd1306_command(SSD1306_SETCONTRAST);
  display.ssd1306_command(50);
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(20, 10);
  display.println(F("Speedometer v1.0"));
  display.display();
  delay(2000);  // 2 seconds to allow uploads

  set_sleep_mode(SLEEP_MODE_IDLE);  // Set sleep mode to idle
}

// ==== Read analog voltage ====
float readVoltage(int pin) {
  analogRead(pin);                 // throw first reading away
  delayMicroseconds(5);           // let ADC settle
  int analogValue = analogRead(pin);
  return analogValue * (5 / 1023.0); 
}

// ==== Display speed on OLED ====
void displaySpeed(unsigned long spd) {
  static unsigned long lastSpd = 99999;  // store previous speed to prevent redraws

  if (spd != lastSpd) {
    lastSpd = spd;

    // Clear only the speed area (x, y, width, height)
    display.fillRect(0, 0, SCREEN_WIDTH, 40, BLACK);

    display.setTextSize(4);
    display.setTextColor(WHITE);
    display.setCursor(0, 10);
    display.println(spd);

    display.setTextSize(2);
    display.setCursor(0, 48); // Position label below number
    display.print(F("KM/h"));

    display.display();
  }
}
// ==== Update speed based on time ====
void updateSpeed(unsigned long timeDelta) {
  if (timeDelta > 0) {
    speed = (2200UL * 3600UL) / timeDelta;  // speed in mm/h

    Serial.print(speed / 1000);
    Serial.print(" km/h, Distance: ");
    Serial.print(totalDistance_mm / 1000);  // show meters
    Serial.println(" m");

    displaySpeed(speed / 1000);  // display in km/h
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
      totalDistance_mm += 2200;  // Add 1 full wheel revolution distance
      aboveThreshold = true;
    }
  } else {
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


// 4.4V battery @ 20mA ~ 0.088W
// Sensor doesnt measure as accurate as with USB power, needs stronger field before activation. 
// Maybe can change threshold voltage, use a better battery, or wait for a digital sensor.
