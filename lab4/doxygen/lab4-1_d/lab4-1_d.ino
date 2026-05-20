/**
 * @file lab4-1_d.ino
 * @author Ruiqi Liu, Hailey Yuan, Gemini 404
 * @date 05/22/2026
 * @brief Implements a Shortest Time Remaining First (STRF) cooperative scheduler using FreeRTOS on an ESP32.
 * * Three tasks run on Core 0 — blinking an LED, printing an incrementing counter to an I2C LCD, 
 * and printing the alphabet to Serial. A dedicated scheduler task selects whichever job has the 
 * least remaining execution time and runs it exclusively until all three finish, then resets and repeats.
 * @
 */

// ================== Includes ==================
#include <LiquidCrystal_I2C.h>

// ================== Macros ==================
#define LED_PIN 4   ///< GPIO pin connected to the external LED

// ================== Global Variables ==================

/** Total execution durations for each task, expressed in FreeRTOS ticks. Used to reset remaining times. */
const TickType_t totalTimes[3] = {48000 / portTICK_PERIOD_MS, 20000 / portTICK_PERIOD_MS, 26000 / portTICK_PERIOD_MS};
/** Countdown timers tracking how much execution time each task still has left. */
volatile TickType_t remainingTimes[3] = {48000 / portTICK_PERIOD_MS, 20000 / portTICK_PERIOD_MS, 26000 / portTICK_PERIOD_MS};
/** Task handles allow the scheduler to suspend/resume individual tasks by reference. */
TaskHandle_t taskHandles[3];
/** Whether each task is currently sleeping/waiting. */
volatile bool isSleeping[3] = {false, false, false};

/** LCD object using I2C address 0x27, configured for a 16-column × 2-row display. */
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ================== Functions ==================
/**
 * @brief Function that delays a task.
 * * @param ms Number of milliseconds to delay
 * * @param index Index of the task to delay
 * * @return void
 */
void taskDelay(int ms, int index) {
   isSleeping[index] = true;
   vTaskDelay(ms / portTICK_PERIOD_MS);
   isSleeping[index] = false;
   remainingTimes[index] -= ms / portTICK_PERIOD_MS;
}

/**
 * @brief FreeRTOS task that blinks an LED in a fixed pattern.
 * * Blinks an LED (2 s ON, 0.95 s OFF, 0.10 s ON, 0.95 s OFF) repeated 12 times for a
 * total of 48 seconds. After completing one full cycle the task suspends itself, 
 * waiting for the scheduler to resume it in the next round.
 * * @param* arg Unused task parameter (required by FreeRTOS API)
 * * @return void (FreeRTOS tasks never return)
 */
void ledTask(void *arg) {
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

/**
 * @brief FreeRTOS task that counts from 1 to 20.
 * * Prints each value on the first row of the LCD once per second, for a total of 20 seconds.
 * After reaching 20 the task suspends itself until the scheduler resumes it for the next cycle.
 * * @param arg Unused task parameter (required by FreeRTOS API)
 * @return void (FreeRTOS tasks never return)
 */
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

/**
 * @brief FreeRTOS task that prints each letter A–Z to the Serial monitor.
 * * Prints at a rate of one letter per second (26 seconds total). After 'Z'
 * the task suspends itself until the scheduler resumes it.
 * * @param arg Unused task parameter (required by FreeRTOS API)
 * @return void (FreeRTOS tasks never return)
 */
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

/**
 * @brief FreeRTOS task that implements a Shortest Job First (SJF) scheduler.
 * * Every 100 ms it evaluates the remaining execution time of the three worker tasks, 
 * resumes whichever has the least time left (and is not yet done), and suspends the others.
 * When all three remaining times reach zero, it resets the counters and begins the next cycle.
 * * @param arg Unused task parameter (required by FreeRTOS API)
 * @return void (FreeRTOS tasks never return)
 */
void scheduleTasks(void *arg) {
   while (1) {
      if (remainingTimes[0] <= 0 && remainingTimes[1] <= 0 && remainingTimes[2] <= 0) {
         // Reset the remaining times back to their initial execution times
         for (int i = 0; i < 3; i++) {
            remainingTimes[i] = totalTimes[i];
            isSleeping[i] = false;
         }
      }

      TickType_t minTime = 0xFFFFFFFF; // Set to absolute maximum initially
      int shortestTask = -1;           // 0: LED, 1: Counter, 2: Alphabet
      for (int i = 0; i < 3; i++) {
         TickType_t time = remainingTimes[i];
         if (time > 0 && !isSleeping[i] && time < minTime) {
            minTime = time;
            shortestTask = i;
         }
      }

      if (shortestTask != -1) {
         vTaskResume(taskHandles[shortestTask]);
         for (int i = 0; i < 3; i++) {
            if (i != shortestTask && remainingTimes[i] > 0 && !isSleeping[i]) {
               vTaskSuspend(taskHandles[i]);
            }
         }
      }

      vTaskDelay(100 / portTICK_PERIOD_MS);
   }
}

/**
 * @brief Standard Arduino setup function to initialize hardware and FreeRTOS tasks.
 */
void setup() {
   Serial.begin(115200);

   pinMode(LED_PIN, OUTPUT);

   lcd.init();
   lcd.backlight();
   lcd.setCursor(0, 0);
   lcd.clear();

   xTaskCreatePinnedToCore(ledTask, "LED Task", 2048, NULL, 1, &taskHandles[0], 0);
   xTaskCreatePinnedToCore(counterTask, "Counter Task", 2048, NULL, 1, &taskHandles[1], 0);
   xTaskCreatePinnedToCore(alphabetTask, "Alphabet Task", 2048, NULL, 1, &taskHandles[2], 0);
   xTaskCreatePinnedToCore(scheduleTasks, "Scheduler Task", 2048, NULL, 2, NULL, 0);
}

/**
 * @brief Standard Arduino loop function (Empty because FreeRTOS is handling execution).
 */
void loop() {}