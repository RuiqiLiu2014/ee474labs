// Filename: lab2-2.ino
// Author: Ruiqi Liu, Hailey Yuan
// Date: 04/20/2026
// Description: Measures times of direct register access and Arduino library functions.
// Lab 2 step 2

// Gemini-909 (no debugging needed)

#include "driver/gpio.h"
#include "soc/io_mux_reg.h"
#include "soc/gpio_reg.h"
#include "soc/gpio_periph.h"

// Changed to GPIO 2 (safe on ESP32). GPIO 8 is usually connected to internal Flash and will crash the ESP32!
const int testPin = 2; 

void setup() {
    Serial.begin(9600); // Initialize serial communication
    pinMode(testPin, OUTPUT); // Set the pin as output
}

void loop() {
    unsigned long startTime, endTime, libTime, regTime;

    // Measure time for library function (digitalWrite)
    startTime = micros();
    for (int i = 0; i < 1000; i++) {
        digitalWrite(testPin, HIGH); // Turn pin's output to HIGH
        digitalWrite(testPin, LOW);  // Turn pin's output to LOW
    }
    endTime = micros();
    libTime = endTime - startTime;

    // Measure time for direct register access
    startTime = micros();
    for (int i = 0; i < 1000; i++) {
        *(volatile uint32_t *)GPIO_OUT_REG |= (1 << testPin);  // Turn pin's output to HIGH
        *(volatile uint32_t *)GPIO_OUT_REG &= ~(1 << testPin); // Turn pin's output to LOW
    }
    endTime = micros();
    regTime = endTime - startTime;

    // Print out total time to the serial monitor
    Serial.print("Library digitalWrite time (us): ");
    Serial.println(libTime);
    Serial.print("Direct register time (us): ");
    Serial.println(regTime);
    Serial.println();

    delay(1000); // 1 second delay
}