#include <Wire.h>
#include <avr/sleep.h>
#include <SD.h>
#include <U8g2lib.h>

// ==== OLED Setup ====
U8G2_SSD1306_128X64_NONAME_1_HW_I2C display(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// ==== Globals ====
bool aboveThreshold = false;
unsigned long lastTriggerTime = 0;
uint16_t deltaT = 0;
unsigned long speed = 0;
unsigned long totalDistance_mm = 0;

// ==== Logging buffer ====
#define LOG_ENTRIES_BEFORE_FLUSH 20
#define LOG_BUFFER_SIZE (LOG_ENTRIES_BEFORE_FLUSH * 2)

uint8_t logBuffer[LOG_BUFFER_SIZE];
uint16_t logIndex = 0;

extern int __heap_start, *__brkval;
int freeMemory() {
  int v;
  return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
}

// ==== Setup ====
void setup() {
  Serial.begin(9600);

  // U8g2 init
  display.begin();
  display.clearBuffer();

  // ==== SD card detection ====
  display.firstPage();
  do {
    display.setFont(u8g2_font_6x12_tf);
    display.drawStr(0, 28, SD.begin(10) ? "> SD card detected" : "> No SD card");
  } while (display.nextPage());


  delay(2000);

  set_sleep_mode(SLEEP_MODE_IDLE);
}

// ==== Read analog voltage ====
int readVoltage(int pin) {
  analogRead(pin); delayMicroseconds(5);
  return analogRead(pin); // raw 0–1023
}

// ==== Display speed on OLED ====
void displaySpeed(unsigned long spd) {
  static unsigned long lastSpd = 99999;
  if (spd != lastSpd) {
    lastSpd = spd;

    char buf[16];
    snprintf(buf, sizeof(buf), "%lu", spd);

    display.firstPage();
    do {
      // Speed
      display.setFont(u8g2_font_logisoso24_tr);
      display.drawStr(0, 50, buf);

      // Unit
      display.setFont(u8g2_font_6x12_tf);
      display.drawStr(90, 50, "km/h");
    } while (display.nextPage());
  }
}

// ==== Update speed ====
void updateSpeed(unsigned long timeDelta) {
  if (timeDelta > 0) {
    speed = (2200UL * 3600UL) / timeDelta;
    displaySpeed(speed / 1000);
  }
}

void printBufferAsHex(const char* buffer, int length) {
 for (int i = 0; i < length; i++) {
   if ((uint8_t)buffer[i] < 16) Serial.print('0');
   Serial.print((uint8_t)buffer[i], HEX);
 Serial.print(' ');
 }
 Serial.println();
}

// then call it like this
void flushLogBuffer() {
  if (logIndex == 0) return;  // Nothing to flush

  printBufferAsHex(logBuffer, logIndex);
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
      totalDistance_mm += 2200;

      bufferDeltaT(deltaT);
      aboveThreshold = true;
    }
  } else {
    aboveThreshold = false;
  }
}

// ==== Main loop ====
void loop() {
  int voltage = readVoltage(A0);
  handleThresholdCrossing(voltage, 614);

  if ((millis() - lastTriggerTime) > 10000 && speed != 0) {
    speed = 0;
    displaySpeed(0);
  }

  sleep_enable();
  sleep_cpu();
  sleep_disable();
}


// 4.4V battery @ 20mA ~ 0.088W
// Sensor doesnt measure as accurate as with USB power, needs stronger field before activation. 
// Maybe can change threshold voltage, use a better battery, or wait for a digital sensor.

//1673 bytes (81%) of dynamic memory, leaving 375 bytes for local variables. Maximum is 2048 bytes. BUFFER = 5
//1703 bytes (83%) of dynamic memory, leaving 345 bytes for local variables. Maximum is 2048 bytes. BUFFER = 20
