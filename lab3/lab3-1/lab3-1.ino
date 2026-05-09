// Filename: lab3-1.ino
// Authors: Ruiqi Liu, Hailey Yuan
// Date: 5/8/2026
// Description: This file allows users to write text to the serial monitor and display it on the LCD.
// Gemini-909

// INCLUDES

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// MACROS

#define SDA_PIN 8
#define SCL_PIN 9

// GLOBAL VARIABLES

LiquidCrystal_I2C lcd(0x27, 16, 2); // Initialize the LCD
const int LCD_ADDR = 0x27; // The I2C address of the LCD

// Bitmasks
const uint8_t RS_DATA  = 0x01; // Bit 0: 1 = Data, 0 = Command
const uint8_t RW_WRITE = 0x00; // Bit 1: 0 = Write (always 0 for this lab)
const uint8_t EN_BIT   = 0x04; // Bit 2: Enable bit
const uint8_t BL_ON    = 0x08; // Bit 3: Backlight control (1 = On)

// FUNCTIONS

// Name: sendNibble
// Description: sends 4 data bits and control bits to the LCD
// nibble: the bits being sent
// isData: determines if we are sending data (text) or a command (cursor position, clear)
void sendNibble(uint8_t nibble, bool isData) {
  uint8_t rs = isData ? RS_DATA : 0x00; 
  
  // Combine the data nibble (bits 4-7) with backlight, R/W, and RS bits
  uint8_t packet = nibble | BL_ON | RW_WRITE | rs;

  // Send packet with Enable bit HIGH
  Wire.beginTransmission(LCD_ADDR);
  Wire.write(packet | EN_BIT); 
  Wire.endTransmission();

  // Small delay to allow the LCD to detect the HIGH signal
  delayMicroseconds(1);

  // Send packet with Enable bit LOW (This "latches" the data into the LCD)
  Wire.beginTransmission(LCD_ADDR);
  Wire.write(packet & ~EN_BIT); 
  Wire.endTransmission();

  // Small delay to allow the LCD to process the data
  delayMicroseconds(50); 
}

// Name: sendByte
// Description: Sends a byte (8 bits) to the LCD. The byte is broken into two nibbles.
// value: the byte being sent
// isData: determines if we are sending data (text) or a command (cursor position, clear)
void sendByte(uint8_t value, bool isData) {
  // Send High Nibble first (Bits 4-7)
  sendNibble(value & 0xF0, isData);
  
  // Send Low Nibble second (Shift bits 0-3 up to the 4-7 position)
  sendNibble((value << 4) & 0xF0, isData);
}

// SETUP AND LOOP

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);
  
  // We use the library here just to handle the complex 4-bit initialization sequence
  lcd.init();
  delay(2);
  
  Serial.println("Setup Complete. Type a message in the Serial Monitor:");
}

void loop() {
  // Check if there is text waiting in the Serial Monitor
  if (Serial.available() > 0) {
    // Read the incoming message until the user presses enter
    String input = Serial.readStringUntil('\n');
    input.trim(); // Remove any extra hidden spacing or carriage returns (\r)

    if (input.length() > 0) {
      // Clear the screen (Command 0x01)
      sendByte(0x01, false); // false = command
      delay(2);              // Clear command takes longer to execute (~2ms)

      // Set cursor to the beginning of the first line (Command 0x80)
      sendByte(0x80, false); // false = command

      // Loop through the string characters and send them one by one
      for (int i = 0; i < input.length(); i++) {
        // If we reach the 17th character (index 16), jump to the second line
        if (i == 16) {
          sendByte(0xC0, false); // 0xC0 is 0x80 | 0x40 (Line 2 command)
        }
        
        // If the string is longer than 32 characters, stop printing
        if (i == 32) {
          break; 
        }

        // Send the individual character to be displayed
        sendByte(input[i], true); // true = data
      }
    }
  }
}