// Filename: lab4-1.ino
// Author: Ruiqi Liu, Hailey Yuan
// Date: 05/19/2026
/* Description: Implements a Shortest Time Remaining First (STRF) cooperative scheduler using
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
// // Total times for tasks
// const TickType_t ledTaskExecutionTime = 48000 / portTICK_PERIOD_MS;      // 48 seconds
// const TickType_t counterTaskExecutionTime = 20000 / portTICK_PERIOD_MS;  // 20 seconds
// const TickType_t alphabetTaskExecutionTime = 26000 / portTICK_PERIOD_MS; // 26 seconds

// // Remaining Execution Times
// volatile TickType_t remainingLedTime = ledTaskExecutionTime;
// volatile TickType_t remainingCounterTime = counterTaskExecutionTime;
// volatile TickType_t remainingAlphabetTime = alphabetTaskExecutionTime;

// // Task Handles
// TaskHandle_t ledTaskHandle;
// TaskHandle_t counterTaskHandle;
// TaskHandle_t alphabetTaskHandle;

const TickType_t totalTimes[3] = {48000 / portTICK_PERIOD_MS, 20000 / portTICK_PERIOD_MS, 26000 / portTICK_PERIOD_MS};
volatile TickType_t remainingTimes[3] = {48000 / portTICK_PERIOD_MS, 20000 / portTICK_PERIOD_MS, 26000 / portTICK_PERIOD_MS};
TaskHandle_t taskHandles[3];

// Initialize LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ================== Functions ==================
void taskDelay(int ms, int index) {
   vTaskDelay(ms / portTICK_PERIOD_MS)
   remainingTimes[index] -= ms / portTICK_PERIOD_MS;
}

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

    while (1) {
        for (int i = 0; i < 12; i++) {
            digitalWrite(LED_PIN, HIGH);
            taskDelay(2000, 0);

            digitalWrite(LED_PIN, LOW);
            taskDelay(950, 0);

            digitalWrite(LED_PIN, HIGH);
            taskDelay(100, 0);

            digitalWrite(LED_PIN, LOW);
            taskDelay(950, 0);
        }

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

         taskDelay(1000, 1);
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
         taskDelay(1000, 2);
      }
      Serial.println();

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
      if (remainingTimes[0] <= 0 && remainingTimes[1] <= 0 && remainingTimes[2] <= 0) {
         // Reset the remaining times back to their initial execution times
         for (int i = 0; i < 3; i++) {
             remainingTimes[i] = totalTimes[i];
         }
      }

      // Step 2: Find the task with the shortest remaining time > 0
      TickType_t minTime = 0xFFFFFFFF; // Set to absolute maximum initially
      int shortestTask = -1;           // 0: LED, 1: Counter, 2: Alphabet
      for (int i = 0; i < 3; i++) {
         TickType_t time = remainingTimes[i];
         if (time > 0 && time < minTime) {
            minTime = time;
            shortestTask = i;
         }
      }

      if (shortestTask != -1) {
         vTaskResume(taskHandles[shortestTask]);
         vTaskSuspend(taskHandles[(shortestTask + 1) % 3]);
         vTaskSuspend(taskHandles[(shortestTask + 2) % 3]);
      }

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
        &taskHandles[0],     // Pass the global handle so the scheduler can control it
        0                   // Pin to Core 0
    );

    xTaskCreatePinnedToCore(
        counterTask, 
        "Counter Task", 
        2048, 
        NULL, 
        1,                  // Priority: 1
        &taskHandles[1], // Pass the global handle
        0                   // Pin to Core 0
    );

    xTaskCreatePinnedToCore(
        alphabetTask, 
        "Alphabet Task", 
        2048, 
        NULL, 
        1,                  // Priority: 1
        &taskHandles[2],// Pass the global handle
        0                   // Pin to Core 0
    );
}

void loop() {}
