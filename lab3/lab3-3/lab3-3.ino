// Filename: lab3-3.ino
// Authors: Ruiqi Liu, Hailey Yuan
// Date: 5/8/2026
// Description: This file runs ISRs from a button, BLE, and timer.
// Gemini-909

// INCLUDES

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

// MACROS

#define BUTTON_PIN 7
// Generated random UUIDs for BLE Service and Characteristic
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// GLOBAL VARIABLES

// Initialize the LCD display
LiquidCrystal_I2C lcd(0x27, 16, 2); 

volatile bool timer_flag = false;
volatile bool button_flag = false;
volatile bool ble_flag = false;

volatile int counter = 0;
volatile unsigned long last_button_time = 0; // Used for debouncing

// FUNCTIONS

// Name: timer_isr
// Description: updates LCD counter using the timer flag.
void IRAM_ATTR timer_isr() {
  counter++;           // Increment the counter
  timer_flag = true;   // Tell the main loop it's time to update the LCD
}

// Name: button_isr
// Description: Sets a flag when the button is pressed.
void IRAM_ATTR button_isr() {
  // Simple software debounce to prevent multiple triggers from one press
  unsigned long current_time = millis(); 
  if (current_time - last_button_time > 200) { 
    button_flag = true; // Tell main loop the button was pressed
    last_button_time = current_time;
  }
}

// Class to handle interrupts from the LightBlue app
class MyCallbacks: public BLECharacteristicCallbacks {
  // Name: onWrite
  // Description: Sets a flag when a signal is recieved using BLE.
  void onWrite(BLECharacteristic *pCharacteristic) {
    // When a signal is received from the LightBlue app, set the flag
    ble_flag = true; 
  }
};

// SETUP AND LOOP

hw_timer_t * timer = NULL; // Pointer to our hardware timer

void setup() {
  Serial.begin(115200);

  // =========> Initialize LCD display
  lcd.init();
  lcd.backlight();
  lcd.print("BLE Starting...");

  // Initialize BLE
  BLEDevice::init("whuck"); // Name that will appear in LightBlue app
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );

  pCharacteristic->setCallbacks(new MyCallbacks());
  pService->start();
  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->start();

  // Initialize Button Interrupt
  pinMode(BUTTON_PIN, INPUT_PULLUP); 
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), button_isr, FALLING);

  // Initialize Timer Interrupt
  // timerBegin(frequency): 1,000,000 Hz gives us a 1 microsecond resolution
  timer = timerBegin(1000000); 
  timerAttachInterrupt(timer, &timer_isr);
  
  timerAlarm(timer, 1000000, true, 0); 
  
  lcd.clear();
}

void loop() {
  // Check if Button was pressed
  if (button_flag) {
    button_flag = false; // Reset flag
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Button Pressed");
    
    // Pause the LCD for 2 seconds. 
    // The timer ISR is STILL running in the background incrementing the counter!
    delay(2000); 
    lcd.clear();
    timer_flag = true; // Force LCD to immediately reprint the updated counter
  }

  // Check if BLE message was received
  if (ble_flag) {
    ble_flag = false; // Reset flag
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("New Message!");
    
    // Pause the LCD for 2 seconds.
    delay(2000);
    lcd.clear();
    timer_flag = true; // Force LCD to immediately reprint the updated counter
  }

  // Check if Timer went off (Updates every 1 second)
  if (timer_flag) {
    timer_flag = false; // Reset flag
    lcd.setCursor(0, 0);
    lcd.print("Counter: ");
    lcd.print(counter);
    lcd.print("   "); // Print extra spaces to clear any trailing characters
  }
}