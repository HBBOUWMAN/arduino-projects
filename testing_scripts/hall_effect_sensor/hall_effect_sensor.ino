// ==== Setup ====
void setup() {
  Serial.begin(9600);
  pinMode(5, OUTPUT);
}

// ==== Globals ====
bool aboveThreshold = false;
unsigned long lastTriggerTime = 0;
unsigned long timeBetweenTriggers = 0;
unsigned long speed = 0;

// ==== Read analog voltage ====
float readVoltage(int pin) {
  analogRead(pin);  // throw first reading away
  delayMicroseconds(5); // Let ADC settle if needed
  int analogValue = analogRead(pin);
  return analogValue * (5.0 / 1023.0);
}

// ==== Update speed based on time between triggers ====
void updateSpeed(unsigned long timeDelta) {
  if (timeDelta > 0) {  // prevent division by zero
    speed = 2200 / timeDelta;
    Serial.print(speed);
    Serial.println("KM/h");
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
  float voltage = readVoltage(A0);  // read as often as possible
  handleThresholdCrossing(voltage, 3.0);
  
  // No delay here: the loop runs continuously
}