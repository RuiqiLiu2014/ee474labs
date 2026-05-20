// Filename: lab4-2.ino
// Author: Ruiqi Liu, Hailey Yuan
// Date: 05/19/2026
/* Description: Implements a multi-core FreeRTOS application on an ESP32 that
                monitors ambient light via a photoresistor. Core 0 runs two
                cooperative tasks: one reads the sensor and computes a 10-sample
                Simple Moving Average (SMA), and a second displays the current
                reading and SMA on an I2C LCD. Core 1 runs two independent tasks:
                one flashes an LED alarm when the SMA exceeds defined thresholds,
                and another continuously finds and prints prime numbers up to 5000.
                A binary semaphore protects the shared light-level data between
                the sensor and display tasks. */

// Gemini 305

// ================== Includes ==================
#include <LiquidCrystal_I2C.h>

// ================== Macros ==================
#define LED_PIN 4
#define PHOTO_PIN 7

// ================== Global Variables ==================
volatile int lightLevel;
volatile int SMA;

SemaphoreHandle_t sema;
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ================== Functions ==================

// Name: calculateAvg
// Description: Computes the arithmetic mean of a 10-element integer array.
//              Used by lightDetectorTask to produce the Simple Moving Average.
// Parameters:  buffer - pointer to an array of exactly 10 integer ADC samples
// Returns:     int - the integer average (truncated, not rounded) of the 10 values
int calculateAvg(int buffer[]) {
  int sum = 0;
  for (int i = 0; i < 10; i++) {
    sum += buffer[i];
  }
  return sum / 10;
}

// Name: lightDetectorTask
// Description: FreeRTOS task (Core 0) that reads the photoresistor every 500 ms,
//              stores the value in a 10-element circular buffer, and updates the
//              global lightLevel and SMA under semaphore protection.
// Parameters:  arg - unused task parameter (required by FreeRTOS API)
// Returns:     void (FreeRTOS tasks never return)
void lightDetectorTask(void *arg) {
  int buffer[10] = {0};
  int index = 0;

  while (1) {
    if (xSemaphoreTake(sema, portMAX_DELAY) == pdTRUE) {
      lightLevel = analogRead(PHOTO_PIN);
      buffer[index] = lightLevel;
      index = (index + 1) % 10;
      SMA = calculateAvg(buffer);
      xSemaphoreGive(sema); // Release immediately
    }
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

// Name: lcdTask
// Description: FreeRTOS task (Core 0) that polls the shared lightLevel and SMA
//              every 250 ms and refreshes the I2C LCD only when either value has
//              changed since the last update, avoiding unnecessary redraws.
// Parameters:  arg - unused task parameter (required by FreeRTOS API)
// Returns:     void (FreeRTOS tasks never return)
void lcdTask(void *arg) {
  int lastLight = -1;
  int lastSMA = -1;

  while (1) {
    int currentLight = 0;
    int currentSMA = 0;

    // Grab the shared variables quickly
    if (xSemaphoreTake(sema, portMAX_DELAY) == pdTRUE) {
      currentLight = lightLevel;
      currentSMA = SMA;
      xSemaphoreGive(sema);
    }

    // Only update if there is a change
    if (currentLight != lastLight || currentSMA != lastSMA) {
      lcd.setCursor(0, 0);
      lcd.print("Light: ");
      lcd.print(currentLight);
      lcd.print("    "); // Overwrite trailing characters
      
      lcd.setCursor(0, 1);
      lcd.print("SMA: ");
      lcd.print(currentSMA);
      lcd.print("    ");

      // Save the new state
      lastLight = currentLight;
      lastSMA = currentSMA;
    }
    
    // Lab requires 250ms check rate for the display
    vTaskDelay(250 / portTICK_PERIOD_MS); 
  }
}

// Name: anomalyAlarmTask
// Description: FreeRTOS task (Core 1) that reads the current SMA under semaphore
//              protection and flashes the alarm LED 3 times in ~1 second whenever
//              the SMA falls outside the normal range (300–3800). A 2-second pause
//              follows each burst to avoid overwhelming the signal. When no anomaly
//              is present, the task sleeps 250 ms before checking again.
// Parameters:  arg - unused task parameter (required by FreeRTOS API)
// Returns:     void (FreeRTOS tasks never return)
void anomalyAlarmTask(void *arg) {
  pinMode(LED_PIN, OUTPUT);

  while (1) {
    int currentSMA = 0;

    // Grab the SMA quickly and release the semaphore
    if (xSemaphoreTake(sema, portMAX_DELAY) == pdTRUE) {
      currentSMA = SMA;
      xSemaphoreGive(sema);
    }

    // Now process the delays OUTSIDE the semaphore lock
    if (currentSMA > 3800 || currentSMA < 300) {
      
      // Flashes 3 times within 1 second
      for (int i = 0; i < 3; i++) {
        digitalWrite(LED_PIN, HIGH);
        vTaskDelay(166 / portTICK_PERIOD_MS); 
        digitalWrite(LED_PIN, LOW);
        vTaskDelay(166 / portTICK_PERIOD_MS);
      }
      // 2-second pause between bursts
      vTaskDelay(2000 / portTICK_PERIOD_MS); 
    } else {
      vTaskDelay(250 / portTICK_PERIOD_MS); 
    }
  }
}

// Name: isPrime
// Description: Determines whether a given integer is a prime number using trial
//              division up to the square root of n, which is sufficient to prove
//              primality without checking all divisors up to n.
// Parameters:  n - the integer to test (values ≤ 1 always return false)
// Returns:     bool - true if n is prime, false otherwise
bool isPrime(int n) {
    if (n <= 1) return false;

    for (int i = 2; i * i <= n; i++) {
        if (n % i == 0) {
            return false;
        }
    }

    return true;
}

// Name: primeCalculationTask
// Description: FreeRTOS task (Core 1) that iterates from 2 to 5000, prints each
//              prime to the Serial monitor, then pauses 5 seconds before repeating.
//              Operates entirely independently — no semaphore needed because it
//              never accesses the shared light-level variables.
//              A 1 ms yield per iteration prevents starving the FreeRTOS IDLE task
//              and triggering the watchdog timer during heavy computation.
// Parameters:  arg - unused task parameter (required by FreeRTOS API)
// Returns:     void (FreeRTOS tasks never return)
void primeCalculationTask(void *arg) {
  while(1) {
    for (int n = 2; n <= 5000; n++) {
      if (isPrime(n)) {
        Serial.printf("%d ", n);
      }
      
      // Tiny delay to prevent triggering the FreeRTOS watchdog timer on Core 1 
      // by starving the IDLE task during heavy computation
      vTaskDelay(1 / portTICK_PERIOD_MS); 
    }
    
    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(PHOTO_PIN, INPUT);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.clear();

  sema = xSemaphoreCreateBinary();

  if (sema == NULL) {
    exit(EXIT_FAILURE);
  }

  xSemaphoreGive(sema);

  xTaskCreatePinnedToCore(
        lightDetectorTask,      // Function name
        "Light Detector Task",   // Human-readable name for debugging
        2048,               // Stack size (2048 bytes is safe for most logic)
        NULL,               // Task parameter (unused)
        1,                  // Priority: 2 (Highest)
        NULL,               // Task Handle (We don't need to suspend the scheduler, so NULL)
        0                   // Pin to Core 0
  );

  xTaskCreatePinnedToCore(
        lcdTask,      // Function name
        "LCD Task",   // Human-readable name for debugging
        2048,               // Stack size (2048 bytes is safe for most logic)
        NULL,               // Task parameter (unused)
        1,                  // Priority: 2 (Highest)
        NULL,               // Task Handle (We don't need to suspend the scheduler, so NULL)
        0                   // Pin to Core 0
  );

  xTaskCreatePinnedToCore(
        anomalyAlarmTask,      // Function name
        "Anomaly Alarm Task",   // Human-readable name for debugging
        2048,               // Stack size (2048 bytes is safe for most logic)
        NULL,               // Task parameter (unused)
        1,                  // Priority: 2 (Highest)
        NULL,               // Task Handle (We don't need to suspend the scheduler, so NULL)
        1                   // Pin to Core 1
  );

  xTaskCreatePinnedToCore(
        primeCalculationTask,      // Function name
        "Prime Calculation Task",   // Human-readable name for debugging
        2048,               // Stack size (2048 bytes is safe for most logic)
        NULL,               // Task parameter (unused)
        1,                  // Priority: 2 (Highest)
        NULL,               // Task Handle (We don't need to suspend the scheduler, so NULL)
        1                   // Pin to Core 1
  );
}

void loop() {}