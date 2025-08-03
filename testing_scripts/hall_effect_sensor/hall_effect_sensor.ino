void setup() {
  Serial.begin(9600);
  pinMode(5, OUTPUT);
}

float voltage = 0;
bool aboveThreshold = false;
unsigned long lastTriggerTime = 0;
float timeBetweenTriggers = 0;
float speed = 0;

void loop() {
  int analogValue = analogRead(A0);
  voltage = analogValue * (5.0 / 1023.0);
  //Serial.print("Voltage: ");
  //Serial.println(voltage);

  // Check for crossing the 3V threshold
  if (voltage > 3) {
    if (!aboveThreshold) {
      // We just crossed the threshold now
      unsigned long currentTime = millis();
      timeBetweenTriggers = currentTime - lastTriggerTime;
      lastTriggerTime = currentTime;
      speed = 2/(timeBetweenTriggers/1000);

      Serial.print(speed);
      Serial.println("KM/h");

      aboveThreshold = true; // Mark that we're now above
    }

    digitalWrite(5, HIGH); // LED on
  } else {
    digitalWrite(5, LOW);  // LED off
    aboveThreshold = false; // Reset state so we can detect next crossing
  }

  delay(10);
}