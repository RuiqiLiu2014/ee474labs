// Gemini 309

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define LED_PIN 4
#define PHOTO_PIN 7

volatile int lightLevel;
volatile int SMA;

SemaphoreHandle_t sema;
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Light Detector Task (Core 0)
// ====================> TODO:
//          1. Initialize Variables
//          2. Loop Continuously
//           - Read light level from the photoresistor.
//           - Take semaphore
//           - Calculate the simple moving average and update variables.
//           - Give semaphore to signal data is ready.
void lightDetectorTask(void *arg) {
  pinMode(PHOTO_PIN, INPUT);
  int buffer[10] = {0};
  int index = 0;

  while (1) {
    // Read fresh data inside the loop
    int currentLight = analogRead(PHOTO_PIN); 

    if (xSemaphoreTake(sema, portMAX_DELAY) == pdTRUE) {
      lightLevel = currentLight;
      buffer[index] = lightLevel;
      index = (index + 1) % 10;
      SMA = calculateAvg(buffer);
      xSemaphoreGive(sema); // Release immediately
    }
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

int calculateAvg(int buffer[]) {
  int sum = 0;
  for (int i = 0; i < 10; i++) {
    sum += buffer[i];
  }
  return sum / 10;
}

// LCD Task (Core 0)
// ====================> TODO:
//          1. Initialize Variables
//           2. Loop Continuously
//            - Wait for semaphore.
//            - If data has changed, update the LCD with the new light level and SMA.
//            - Give back the semaphore.
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

// Anomaly Alarm Task (Core 1)
// ====================> TODO:
//            1. Loop Continuously
//             - Wait for semaphore.
//             - Check if SMA indicates a light anomaly (outside thresholds).
//             - If an anomaly is detected, flash a LED signal.
//             - Give back the semaphore.
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
      for(int i = 0; i < 3; i++) {
        digitalWrite(LED_PIN, HIGH);
        vTaskDelay(166 / portTICK_PERIOD_MS); 
        digitalWrite(LED_PIN, LOW);
        vTaskDelay(166 / portTICK_PERIOD_MS);
      }
      // 2-second pause between bursts
      vTaskDelay(2000 / portTICK_PERIOD_MS); 

    } else {
      // If no anomaly, delay for display time (250ms) before checking again
      // Prevents the while(1) loop from hogging Core 1
      vTaskDelay(250 / portTICK_PERIOD_MS); 
    }
  }
}

// Prime Calculation Task (Core 1)
// ====================> TODO:
//            1. Loop from 2 to 5000
//             - Check if the current number is prime.
//             - If prime, print the number to the serial monitor.
void primeCalculationTask(void *arg) {
  while(1) {
    // NO SEMAPHORES HERE. This operates completely independently!
    for (int n = 2; n <= 5000; n++) {
      if (isPrime(n)) {
        Serial.printf("%d ", n);
      }
      
      // Tiny delay to prevent triggering the FreeRTOS watchdog timer on Core 1 
      // by starving the IDLE task during heavy computation
      vTaskDelay(1 / portTICK_PERIOD_MS); 
    }
    
    // Optional: Pause before restarting the calculation
    vTaskDelay(5000 / portTICK_PERIOD_MS); 
  }
}

bool isPrime(int n) {
    if (n <= 1) return false;

    for (int i = 2; i * i <= n; i++) {
        if (n % i == 0) {
            return false;
        }
    }

    return true;
}

void setup() {
  Serial.begin(115200);

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
// ====================> TODO:
//         1. Initialize pins, serial, LCD, etc
//         2. Create binary semaphore for synchronization of light level data.
//         3. Create Tasks
//          - Create the `Light Detector Task` and assign it to Core 0.
//          - Create `LCD Task` and assign it to Core 0.
//          - Create `Anomaly Alarm Task` and assign it to Core 1.
//          - Create `Prime Calculation Task` and assign it to Core 1.
void loop() {}