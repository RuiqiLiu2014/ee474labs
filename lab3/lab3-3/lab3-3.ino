#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

// ==========================================
// 1. PIN DEFINITIONS & UUIDS
// ==========================================
#define BUTTON_PIN 4 // Connect a tactile switch from GPIO 4 to GND

// Generated random UUIDs for BLE Service and Characteristic
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// Initialize the LCD display
LiquidCrystal_I2C lcd(0x27, 16, 2); 

// ==========================================
// 2. VOLATILE VARIABLES FOR ISRs
// ==========================================
// 'volatile' tells the compiler these variables can change unexpectedly in an interrupt
volatile bool timer_flag = false;
volatile bool button_flag = false;
volatile bool ble_flag = false;

volatile int counter = 0;
volatile unsigned long last_button_time = 0; // Used for debouncing

// ==========================================
// 3. INTERRUPT SERVICE ROUTINES (ISRs)
// ==========================================
// IRAM_ATTR forces the compiled code into internal RAM for faster execution

// --- Timer ISR ---
void IRAM_ATTR timer_isr() {
  counter++;           // Increment the counter
  timer_flag = true;   // Tell the main loop it's time to update the LCD
}

// --- Button ISR ---
void IRAM_ATTR button_isr() {
  // Simple software debounce to prevent multiple triggers from one press
  unsigned long current_time = millis(); 
  if (current_time - last_button_time > 200) { 
    button_flag = true; // Tell main loop the button was pressed
    last_button_time = current_time;
  }
}

// --- BLE Callback (Interrupt-driven behind the scenes) ---
class MyCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    // When a signal is received from the LightBlue app, set the flag
    ble_flag = true; 
  }
};

// ==========================================
// 4. SETUP
// ==========================================
hw_timer_t * timer = NULL; // Pointer to our hardware timer

void setup() {
  Serial.begin(115200);

  // =========> Initialize LCD display
  lcd.init();
  lcd.backlight();
  lcd.print("BLE Starting...");

  // =========> Initialize BLE
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

  // =========> Initialize Button Interrupt
  // Use INPUT_PULLUP so we don't need an external resistor. Button connects to GND.
  pinMode(BUTTON_PIN, INPUT_PULLUP); 
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), button_isr, FALLING);

  // =========> Initialize Timer Interrupt (ESP32 Core 3.x Syntax)
  // timerBegin(frequency): 1,000,000 Hz gives us a 1 microsecond resolution
  timer = timerBegin(1000000); 
  timerAttachInterrupt(timer, &timer_isr);
  
  // timerAlarm(timer, alarm_value, auto_reload, reload_value)
  // 1,000,000 microseconds = 1 second
  timerAlarm(timer, 1000000, true, 0); 
  
  lcd.clear();
}

// ==========================================
// 5. MAIN LOOP (LCD Handler)
// ==========================================
void loop() {
  
  // 1. Check if Button was pressed
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

  // 2. Check if BLE message was received
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

  // 3. Check if Timer went off (Updates every 1 second)
  if (timer_flag) {
    timer_flag = false; // Reset flag
    lcd.setCursor(0, 0);
    lcd.print("Counter: ");
    lcd.print(counter);
    lcd.print("   "); // Print extra spaces to clear any trailing characters
  }
}