#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h> // issue is that these library functions take up 1000 bytes of SRAM. Its too much, i need  to use
// https://github.com/greiman/SSD1306Ascii
// or 
//  U8g2
#include <Wire.h>
#include <avr/sleep.h>
#include <SD.h>

// ==== OLED Setup ====
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ==== Globals ====
bool aboveThreshold = false;
unsigned long lastTriggerTime = 0;
uint16_t deltaT = 0;
unsigned long speed = 0;
unsigned long totalDistance_mm = 0;  // in meters

// ==== Logging buffer ====
#define LOG_ENTRIES_BEFORE_FLUSH 5
#define LOG_BUFFER_SIZE (LOG_ENTRIES_BEFORE_FLUSH *2)  // 120 bytes

uint8_t logBuffer[LOG_BUFFER_SIZE];  // ~1000 bytes, safely fits in RAM
uint16_t logIndex = 0;  // use uint16_t here to handle larger buffer sizes


extern int __heap_start, *__brkval;
int freeMemory() {
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

// ==== Setup ====
void setup() {
  Serial.begin(9600);

Serial.print(F("Free RAM before display: "));
Serial.println(freeMemory());

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
  display.println(F("> Speedometer v1.0"));

  // ==== SD card detection ====
  const int chipSelect = 10;  // Adjust if your CS pin is different
  if (SD.begin(chipSelect)) {
    //Serial.println(F("SD card detected"));
    display.setCursor(20, 20);
    display.println(F("> SD card detected"));
  } else {
    //Serial.println(F("No SD card detected"));
    display.setCursor(20, 20);
    display.println(F("> No SD card detected"));
  }


  display.display();
  delay(2000);  // 2 seconds to allow uploads

  set_sleep_mode(SLEEP_MODE_IDLE);  // Set sleep mode to idle
}

// ==== Read analog voltage ====
float readVoltage(int pin) {
  analogRead(pin);                 // throw first reading away
  delayMicroseconds(5);           // let ADC settle
  int analogValue = analogRead(pin);
  return analogValue; //* (5 / 1023.0); // keep as value to improve memory
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

    //display.setTextSize(2);
    //display.setCursor(0, 48); // Position label below number
    //display.print(F("KM/h"));

    display.display();
  }
}

// ==== Update speed based on time ====
void updateSpeed(unsigned long timeDelta) {
  if (timeDelta > 0) {
    speed = (2200UL * 3600UL) / timeDelta;  // speed in mm/h
    displaySpeed(speed / 1000);  // display in km/h
  }
}

//void printBufferAsHex(const char* buffer, int length) {
//  for (int i = 0; i < length; i++) {
//    if ((uint8_t)buffer[i] < 16) Serial.print('0');
//    Serial.print((uint8_t)buffer[i], HEX);
//  Serial.print(' ');
//  }
//  Serial.println();
//}

// then call it like this
void flushLogBuffer() {
  if (logIndex == 0) return;  // Nothing to flush

  //printBufferAsHex(logBuffer, logIndex);
  logIndex = 0;
}

// Buffer one deltaT value as 2 bytes
void bufferDeltaT(uint16_t deltaT) {
  // Check if there's room for 2 more bytes, flush if not
  if ((logIndex + sizeof(uint16_t)) > LOG_BUFFER_SIZE) {
    flushLogBuffer();
  }

  // Copy 2 bytes of deltaT into buffer
  memcpy(logBuffer + logIndex, &deltaT, sizeof(uint16_t));
  logIndex += sizeof(uint16_t);

  // Flush if buffer full (should happen only when full)
  if (logIndex >= LOG_BUFFER_SIZE) {
    flushLogBuffer();
  }
}

// ==== Handle threshold crossing ====
void handleThresholdCrossing(int voltage, int threshold) {
  if (voltage > threshold) {
    if (!aboveThreshold) {
      unsigned long currentTime = millis();
      deltaT = currentTime - lastTriggerTime;
      lastTriggerTime = currentTime;

      updateSpeed(deltaT);
      totalDistance_mm += 2200;  // Add 1 full wheel revolution distance

      bufferDeltaT(deltaT);  // Log deltaT value

      aboveThreshold = true;
    }
  } else {
    aboveThreshold = false;
  }
}

// ==== Main Loop ====
void loop() {
  int voltage = readVoltage(A0);
  handleThresholdCrossing(voltage, 614); // 3v but we assume 3/5v so 1024 * 0.6

  // Check if no trigger for more than 10s
  if ((millis() - lastTriggerTime) > 10000) {
    if (speed != 0) {
      speed = 0;
      displaySpeed(0); // Show zero speed when stopped
    }
  }

  // Enter idle sleep to save power but keep timers running
  sleep_enable();
  sleep_cpu();
  sleep_disable();
}


// 4.4V battery @ 20mA ~ 0.088W
// Sensor doesnt measure as accurate as with USB power, needs stronger field before activation. 
// Maybe can change threshold voltage, use a better battery, or wait for a digital sensor.
