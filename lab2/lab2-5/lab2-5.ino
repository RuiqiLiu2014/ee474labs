// Filename: lab2-5.ino
// Author: Ruiqi Liu, Hailey Yuan
// Date: 04/20/2026
// Description: Plays a buzzing frequency once once the light level read by
// the photoresistor passes a threshold.
// Lab 2 step 5 (part 4)

// Gemini-909 (no debugging needed)

const int photoresistorPin = 7; // Analog input pin connected to the voltage divider
const int buzzerPin = 5;         // Digital output pin connected to the buzzer

// Light threshold: adjust this value based on ambient lighting conditions
// Assuming a dark room corresponds to lower analog readings depending on voltage divider configuration.
const int lightThreshold = 2000; 

// PWM Properties
const int resolution = 8;        // 8-bit resolution (0-255)
const int dutyCycle = 128;       // 50% duty cycle for optimal sound generation

// Timer logic variables
unsigned long sequenceStartTime = 0;
int currentSegment = -1; // -1 indicates the sequence is not playing
bool hasPlayed = false;  // Ensures the sequence plays only once per darkness event

void setup() {
  Serial.begin(115200);
  
  // Initial configuration for the buzzer pin
  ledcAttach(buzzerPin, 500, resolution);
  ledcWrite(buzzerPin, 0); // Ensure buzzer is off initially
}

void loop() {
  int lightLevel = analogRead(photoresistorPin);
  
  // Print light level for debugging and tuning the threshold
  // Serial.print("Light Level: ");
  // Serial.println(lightLevel);

  if (lightLevel < lightThreshold) {
    // Light is below threshold (dark ambient light)
    if (currentSegment == -1 && !hasPlayed) {
      // Start the one-time buzzer sequence
      currentSegment = 0;
      sequenceStartTime = millis();
    }
  } else {
    // Light is above threshold (bright)
    currentSegment = -1;
    hasPlayed = false;       // Reset so it can play again next time it gets dark
    ledcWrite(buzzerPin, 0); // Ensure buzzer remains silent
  }

  // Handle the non-blocking timer logic for the buzzer sequence
  if (currentSegment != -1) {
    unsigned long elapsedTime = millis() - sequenceStartTime;
    
    // First third: play a low frequency (500 Hz)
    if (elapsedTime < 250) {
      if (currentSegment != 1) {
        currentSegment = 1;
        ledcAttach(buzzerPin, 500, resolution);
        ledcWrite(buzzerPin, dutyCycle);
      }
    } 
    // Second third: play a medium frequency (1000 Hz)
    else if (elapsedTime < 500) {
      if (currentSegment != 2) {
        currentSegment = 2;
        ledcAttach(buzzerPin, 1000, resolution);
        ledcWrite(buzzerPin, dutyCycle);
      }
    } 
    // Final third: play a high frequency (2000 Hz)
    else if (elapsedTime < 750) {
      if (currentSegment != 3) {
        currentSegment = 3;
        ledcAttach(buzzerPin, 2000, resolution);
        ledcWrite(buzzerPin, dutyCycle);
      }
    } 
    // Stop after one pass through the sequence
    else {
      currentSegment = -1;
      hasPlayed = true;        // Mark as played to prevent immediate looping
      ledcWrite(buzzerPin, 0); // Turn off the buzzer
    }
  }

  // Short delay for stability
  delay(10);
}
