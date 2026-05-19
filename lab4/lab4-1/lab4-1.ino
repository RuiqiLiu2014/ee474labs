// Filename: lab4-2.ino
// Author: Ruiqi Liu, Hailey Yuan
// Date: 05/19/2026
/* Description: Implements a Shortest Job First (SJF) cooperative scheduler using
             FreeRTOS on an ESP32. Three tasks run on Core 0 — blinking an LED,
             printing an incrementing counter to an I2C LCD, and printing the
             alphabet to Serial. A dedicated scheduler task selects whichever
             job has the least remaining execution time and runs it exclusively
             until all three finish, then resets and repeats. */

// Gemini 404

// ================== Includes ==================
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ================== Macros ==================
#define LED_PIN 4

// ================== Global Variables ==================
// Total times for tasks
const TickType_t ledTaskExecutionTime = 48000 / portTICK_PERIOD_MS;      // 48 seconds
const TickType_t counterTaskExecutionTime = 20000 / portTICK_PERIOD_MS;  // 20 seconds
const TickType_t alphabetTaskExecutionTime = 26000 / portTICK_PERIOD_MS; // 26 seconds
// Remaining Execution Times
volatile TickType_t remainingLedTime = ledTaskExecutionTime;
volatile TickType_t remainingCounterTime = counterTaskExecutionTime;
volatile TickType_t remainingAlphabetTime = alphabetTaskExecutionTime;

TaskHandle_t ledTaskHandle;
TaskHandle_t counterTaskHandle;
TaskHandle_t alphabetTaskHandle;

LiquidCrystal_I2C lcd(0x27, 16, 2);

// ================== Function Prototypes ==================
void ledTask(void *arg);
void counterTask(void *arg);
void alphabetTask(void *arg);
void scheduleTasks(void *arg);

// ================== Function Implementations ==================

// Name: ledTask
// Description: FreeRTOS task that blinks an LED in a fixed pattern (2 s ON,
//              0.95 s OFF, 0.10 s ON, 0.95 s OFF) repeated 12 times for a
//              total of 48 seconds. After completing one full cycle the task
//              suspends itself, waiting for the scheduler to resume it in the
//              next round.
// Parameters:  arg - unused task parameter (required by FreeRTOS API)
// Returns:     void (FreeRTOS tasks never return)
void ledTask(void *arg) {
    pinMode(LED_PIN, OUTPUT);

    // FreeRTOS tasks must run in an infinite loop
    while (1) {
        // Repeat the blink pattern 12 times
        for (int i = 0; i < 12; i++) {
            // ON 2 seconds
            digitalWrite(LED_PIN, HIGH);
            vTaskDelay(2000 / portTICK_PERIOD_MS);
            remainingLedTime -= (2000 / portTICK_PERIOD_MS);

            // OFF 0.95 seconds
            digitalWrite(LED_PIN, LOW);
            vTaskDelay(950 / portTICK_PERIOD_MS);
            remainingLedTime -= (950 / portTICK_PERIOD_MS);

            // ON 0.10 seconds
            digitalWrite(LED_PIN, HIGH);
            vTaskDelay(100 / portTICK_PERIOD_MS);
            remainingLedTime -= (100 / portTICK_PERIOD_MS);

            // OFF 0.95 seconds
            digitalWrite(LED_PIN, LOW);
            vTaskDelay(950 / portTICK_PERIOD_MS);
            remainingLedTime -= (950 / portTICK_PERIOD_MS);
        }

        // Once the 12 iterations (48 seconds) complete, suspend this task.
        // It will remain suspended until your scheduler resets the times and restarts it.
        vTaskSuspend(NULL); 
    }
}


// Name: counterTask
// Description: FreeRTOS task that counts from 1 to 20, printing each value on
//              the first row of the LCD once per second, for a total of 20 seconds.
//              After reaching 20 the task suspends itself until the scheduler
//              resumes it for the next scheduling cycle.
// Parameters:  arg - unused task parameter (required by FreeRTOS API)
// Returns:     void (FreeRTOS tasks never return)
void counterTask(void *arg) {
   while (1) {
      for (int i = 1; i <= 20; i++) {
         lcd.setCursor(0, 0);
         lcd.print("Count: ");
         lcd.print(i);
         lcd.print("    ");

         vTaskDelay(1000 / portTICK_PERIOD_MS);
         remainingCounterTime -= (1000 / portTICK_PERIOD_MS);
      }
      
      vTaskSuspend(NULL);
   }
}

// Name: alphabetTask
// Description: FreeRTOS task that prints each letter A–Z to the Serial monitor
//              at a rate of one letter per second (26 seconds total). After 'Z'
//              the task suspends itself until the scheduler resumes it.
// Parameters:  arg - unused task parameter (required by FreeRTOS API)
// Returns:     void (FreeRTOS tasks never return)
void alphabetTask(void *arg) {
   while (1) {
      for (char c = 'A'; c <= 'Z'; c += 1) {
         Serial.printf("%c ", c);
         vTaskDelay(1000 / portTICK_PERIOD_MS);
         remainingAlphabetTime -= (1000 / portTICK_PERIOD_MS);
      }
      vTaskSuspend(NULL);
   }
}

// Name: scheduleTasks
// Description: FreeRTOS task that implements a non-preemptive Shortest Job First
//              (SJF) scheduler. Every 100 ms it evaluates the remaining execution
//              time of the three worker tasks, resumes whichever has the least
//              time left (and is not yet done), and suspends the others.
//              When all three remaining times reach zero, it resets the counters
//              and immediately begins the next scheduling cycle.
// Parameters:  arg - unused task parameter (required by FreeRTOS API)
// Returns:     void (FreeRTOS tasks never return)
void scheduleTasks(void *arg) {
   while (1) {
      // Step 1: Check if ALL tasks are finished (remaining times are 0 or less)
      if (remainingLedTime <= 0 && remainingCounterTime <= 0 && remainingAlphabetTime <= 0) {
         
         // Reset the remaining times back to their initial execution times
         remainingLedTime = ledTaskExecutionTime;
         remainingCounterTime = counterTaskExecutionTime;
         remainingAlphabetTime = alphabetTaskExecutionTime;
         
         // Note: The tasks suspended themselves when they finished. 
         // We don't need to resume them here; the logic below will 
         // immediately pick the shortest one and resume it.
      }

      // Step 2: Find the task with the shortest remaining time > 0
      TickType_t minTime = 0xFFFFFFFF; // Set to absolute maximum initially
      int shortestTask = -1;           // 0: LED, 1: Counter, 2: Alphabet

      if (remainingLedTime > 0 && remainingLedTime < minTime) {
         minTime = remainingLedTime;
         shortestTask = 0;
      }
      if (remainingCounterTime > 0 && remainingCounterTime < minTime) {
         minTime = remainingCounterTime;
         shortestTask = 1;
      }
      if (remainingAlphabetTime > 0 && remainingAlphabetTime < minTime) {
         minTime = remainingAlphabetTime;
         shortestTask = 2;
      }

      // Step 3: Suspend and Resume tasks based on our finding
      if (shortestTask == 0) {
         vTaskResume(ledTaskHandle);
         vTaskSuspend(counterTaskHandle);
         vTaskSuspend(alphabetTaskHandle);
      } 
      else if (shortestTask == 1) {
         vTaskSuspend(ledTaskHandle);
         vTaskResume(counterTaskHandle);
         vTaskSuspend(alphabetTaskHandle);
      } 
      else if (shortestTask == 2) {
         vTaskSuspend(ledTaskHandle);
         vTaskSuspend(counterTaskHandle);
         vTaskResume(alphabetTaskHandle);
      }

      // Step 4: Delay the scheduler briefly before checking again
      // 100ms is a good polling rate to keep the scheduler responsive 
      // without hogging the CPU.
      vTaskDelay(100 / portTICK_PERIOD_MS);
   }
}


void setup() {
   // TODO: Create 4 tasks and pin them to core 0:
   //          1. A scheduler that handles the scheduling of the other three tasks
   //          2. Blink an LED
   //          3. Print a counter to the LCD
   //          4. Print the alphabet to Serial
   Serial.begin(115200);
   
   lcd.init();
   lcd.backlight();
   lcd.setCursor(0, 0);
   lcd.clear();

   xTaskCreatePinnedToCore(
        scheduleTasks,      // Function name
        "Scheduler Task",   // Human-readable name for debugging
        2048,               // Stack size (2048 bytes is safe for most logic)
        NULL,               // Task parameter (unused)
        2,                  // Priority: 2 (Highest)
        NULL,               // Task Handle (We don't need to suspend the scheduler, so NULL)
        0                   // Pin to Core 0
    );

    xTaskCreatePinnedToCore(
        ledTask, 
        "LED Task", 
        2048, 
        NULL, 
        1,                  // Priority: 1
        &ledTaskHandle,     // Pass the global handle so the scheduler can control it
        0                   // Pin to Core 0
    );

    xTaskCreatePinnedToCore(
        counterTask, 
        "Counter Task", 
        2048, 
        NULL, 
        1,                  // Priority: 1
        &counterTaskHandle, // Pass the global handle
        0                   // Pin to Core 0
    );

    xTaskCreatePinnedToCore(
        alphabetTask, 
        "Alphabet Task", 
        2048, 
        NULL, 
        1,                  // Priority: 1
        &alphabetTaskHandle,// Pass the global handle
        0                   // Pin to Core 0
    );
}

void loop() {}
