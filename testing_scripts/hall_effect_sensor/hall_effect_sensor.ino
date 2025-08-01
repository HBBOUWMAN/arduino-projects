void setup() {
  Serial.begin(9600);       // Start serial communication
  pinMode(5, OUTPUT);       // Set digital pin 5 as output
}

float dt = 0;

void loop() {
  int analogValue = analogRead(A0);               // Read analog input
  float voltage = analogValue * (5.0 / 1023.0);   // Convert to voltage

  Serial.println(voltage);                        // Print voltage

  if (voltage > 3) {
    digitalWrite(5, HIGH);   // Turn LED on
  } else {
    digitalWrite(5, LOW);    // Turn LED off
  }

  delay(100); // Small delay
}