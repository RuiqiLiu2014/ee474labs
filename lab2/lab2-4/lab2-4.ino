// Filename: lab2-4.ino
// Author: Ruiqi Liu, Hailey Yuan
// Date: 04/20/2026
// Description: Plays a buzzing sound depending on the photoresistor reading.
// Lab 2 step 4 (part 3)

// Gemini-909 (no debugging needed)

const int photoresistorPin = 7; // Analog input pin connected to the voltage divider
const int ledPin = 5;            // Digital output pin connected to the LED

// PWM Properties
const int freq = 5000;           // Frequency of 5000 Hz
const int resolution = 8;        // 8-bit resolution, yielding 0-255 duty cycle range

void setup() {
  Serial.begin(115200);
  
  // Configure the PWM signal on the specified GPIO pin
  ledcAttach(ledPin, freq, resolution);
}

void loop() {
  // Read the analog value from the photoresistor (typically 0 - 4095 on ESP32)
  int sensorValue = analogRead(photoresistorPin);
  
  // Map the sensor value to the PWM duty cycle (0 - 255)
  // When light decreases, the resistance of the photoresistor increases, causing
  // the voltage drop across the 10k resistor to decrease (lower analogRead value).
  // We want lower analog values to result in a brighter LED, so we invert the mapping.
  int dutyCycle = map(sensorValue, 0, 4095, 255, 0);
  
  // Constrain to ensure we don't exceed the 8-bit bounds
  dutyCycle = constrain(dutyCycle, 0, 255);
  
  // Update the LED brightness
  ledcWrite(ledPin, dutyCycle);
  
  // Print values to Serial Monitor for debugging
  Serial.print("Analog Reading: ");
  Serial.print(sensorValue);
  Serial.print(" | PWM Duty Cycle: ");
  Serial.println(dutyCycle);
  
  // Short delay for stability
  delay(20);
}
