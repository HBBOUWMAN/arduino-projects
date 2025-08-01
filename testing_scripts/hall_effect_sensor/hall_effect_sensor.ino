void setup() {
  Serial.begin(9600); // Start serial communication at 9600 baud
}

void loop() {
  int analogValue = analogRead(A0); // Read analog value (0-1023)
  
  // Convert to voltage (assuming 5V reference)
  float voltage = analogValue * (5.0 / 1023.0);

  // Print to Serial Monitor
  //Serial.print("Analog value: ");
  //Serial.print(analogValue);
  //Serial.print(" | Voltage: ");
  Serial.println(voltage);
  //Serial.println(" V");

  delay(100); // Wait 500ms before next read
}