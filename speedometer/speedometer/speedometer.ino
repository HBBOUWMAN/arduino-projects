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
const int hallPin = 2; // Digital pin D2

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
  // Hall sensor pin as input with pullup
  pinMode(hallPin, INPUT);
  // U8g2 init
  display.begin();

  // ==== SD card detection ====
  bool sdOk = SD.begin(10);

  display.firstPage();
  do {
    display.setFont(u8g2_font_6x12_tf);
    display.drawStr(0, 12, sdOk ? "SD card detected" : "No SD card");
  } while (display.nextPage());

  delay(2000);
  set_sleep_mode(SLEEP_MODE_IDLE);
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
      display.drawStr(0, 30, buf);

      // Unit
      display.setFont(u8g2_font_6x12_tf);
      display.drawStr(0, 45, "km/h");
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

void flushLogBuffer() {
  if (logIndex == 0) return;  
  printBufferAsHex(logBuffer, logIndex);
  logIndex = 0;
}

void bufferDeltaT(uint16_t deltaT) {
  if ((logIndex + sizeof(uint16_t)) > LOG_BUFFER_SIZE) {
    flushLogBuffer();
  }
  memcpy(logBuffer + logIndex, &deltaT, sizeof(uint16_t));
  logIndex += sizeof(uint16_t);
  if (logIndex >= LOG_BUFFER_SIZE) {
    flushLogBuffer();
  }
}

// ==== Handle threshold crossing (now digital edge detect) ====
void handleHallSensor(bool currentState) {
  // Detect falling edge: HIGH -> LOW
  if (!currentState && aboveThreshold) {
    unsigned long currentTime = millis();
    deltaT = currentTime - lastTriggerTime;
    lastTriggerTime = currentTime;

    updateSpeed(deltaT);
    totalDistance_mm += 2200;

    bufferDeltaT(deltaT);
  }

  // Save current state for next loop
  aboveThreshold = currentState;
}


// ==== Main loop ====
void loop() {
  bool hallState = digitalRead(hallPin);  // HIGH idle, LOW = magnet present
  handleHallSensor(hallState);

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
