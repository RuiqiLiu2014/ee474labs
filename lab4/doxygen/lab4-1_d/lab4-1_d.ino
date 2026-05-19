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
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ================== Macros ==================
#define LED_PIN 4   ///< GPIO pin connected to the external LED

// ================== Global Variables ==================

/** Total execution durations for each task, expressed in FreeRTOS ticks. Used to reset remaining times. */
const TickType_t ledTaskExecutionTime = 48000 / portTICK_PERIOD_MS;      ///< 48 seconds
const TickType_t counterTaskExecutionTime = 20000 / portTICK_PERIOD_MS;  ///< 20 seconds
const TickType_t alphabetTaskExecutionTime = 26000 / portTICK_PERIOD_MS; ///< 26 seconds

/** Countdown timers tracking how much execution time each task still has left. */
volatile TickType_t remainingLedTime = ledTaskExecutionTime;
volatile TickType_t remainingCounterTime = counterTaskExecutionTime;
volatile TickType_t remainingAlphabetTime = alphabetTaskExecutionTime;

/** Task handles allow the scheduler to suspend/resume individual tasks by reference. */
TaskHandle_t ledTaskHandle;
TaskHandle_t counterTaskHandle;
TaskHandle_t alphabetTaskHandle;

/** LCD object using I2C address 0x27, configured for a 16-column × 2-row display. */
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ================== Function Prototypes ==================
void ledTask(void *arg);
void counterTask(void *arg);
void alphabetTask(void *arg);
void scheduleTasks(void *arg);

// ================== Function Implementations ==================

/**
 * @brief FreeRTOS task that blinks an LED in a fixed pattern.
 * * Blinks an LED (2 s ON, 0.95 s OFF, 0.10 s ON, 0.95 s OFF) repeated 12 times for a
 * total of 48 seconds. After completing one full cycle the task suspends itself, 
 * waiting for the scheduler to resume it in the next round.
 * * @param arg Unused task parameter (required by FreeRTOS API)
 * @return void (FreeRTOS tasks never return)
 */
void ledTask(void *arg) {
    pinMode(LED_PIN, OUTPUT);
    while (1) {
        for (int i = 0; i < 12; i++) {
            digitalWrite(LED_PIN, HIGH);
            vTaskDelay(2000 / portTICK_PERIOD_MS);
            remainingLedTime -= (2000 / portTICK_PERIOD_MS);

            digitalWrite(LED_PIN, LOW);
            vTaskDelay(950 / portTICK_PERIOD_MS);
            remainingLedTime -= (950 / portTICK_PERIOD_MS);

            digitalWrite(LED_PIN, HIGH);
            vTaskDelay(100 / portTICK_PERIOD_MS);
            remainingLedTime -= (100 / portTICK_PERIOD_MS);

            digitalWrite(LED_PIN, LOW);
            vTaskDelay(950 / portTICK_PERIOD_MS);
            remainingLedTime -= (950 / portTICK_PERIOD_MS);
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
         vTaskDelay(1000 / portTICK_PERIOD_MS);
         remainingCounterTime -= (1000 / portTICK_PERIOD_MS);
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
         vTaskDelay(1000 / portTICK_PERIOD_MS);
         remainingAlphabetTime -= (1000 / portTICK_PERIOD_MS);
      }
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
      if (remainingLedTime <= 0 && remainingCounterTime <= 0 && remainingAlphabetTime <= 0) {
         remainingLedTime = ledTaskExecutionTime;
         remainingCounterTime = counterTaskExecutionTime;
         remainingAlphabetTime = alphabetTaskExecutionTime;
      }

      TickType_t minTime = 0xFFFFFFFF;
      int shortestTask = -1; // 0: LED, 1: Counter, 2: Alphabet

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

      vTaskDelay(100 / portTICK_PERIOD_MS);
   }
}

/**
 * @brief Standard Arduino setup function to initialize hardware and FreeRTOS tasks.
 */
void setup() {
   Serial.begin(115200);
   lcd.init();
   lcd.backlight();
   lcd.setCursor(0, 0);
   lcd.clear();

   xTaskCreatePinnedToCore(scheduleTasks, "Scheduler Task", 2048, NULL, 2, NULL, 0);
   xTaskCreatePinnedToCore(ledTask, "LED Task", 2048, NULL, 1, &ledTaskHandle, 0);
   xTaskCreatePinnedToCore(counterTask, "Counter Task", 2048, NULL, 1, &counterTaskHandle, 0);
   xTaskCreatePinnedToCore(alphabetTask, "Alphabet Task", 2048, NULL, 1, &alphabetTaskHandle, 0);
}

/**
 * @brief Standard Arduino loop function (Empty because FreeRTOS is handling execution).
 */
void loop() {}