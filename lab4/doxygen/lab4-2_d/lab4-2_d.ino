/**
 * @file lab4-2_d.ino
 * @author Ruiqi Liu, Hailey Yuan, Gemini-305
 * @date 05/22/2026
 * @brief Implements a multi-core FreeRTOS application on an ESP32 that monitors ambient light.
 * * Core 0 runs two cooperative tasks: one reads the sensor and computes a 10-sample Simple Moving 
 * Average (SMA), and a second displays the current reading and SMA on an I2C LCD. Core 1 runs two 
 * independent tasks: one flashes an LED alarm when the SMA exceeds defined thresholds, and another 
 * continuously finds and prints prime numbers up to 5000. A binary semaphore protects the shared 
 * light-level data between the sensor and display tasks.
 */

// ================== Includes ==================
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ================== Macros ==================
#define LED_PIN   4   ///< GPIO pin connected to the alarm LED
#define PHOTO_PIN 7   ///< GPIO pin connected to the photoresistor (analog input)

// ================== Global Variables ==================

/** Shared sensor readings written by lightDetectorTask and read by lcdTask and anomalyAlarmTask. */
volatile int lightLevel; ///< Most recent raw ADC reading from the photoresistor
volatile int SMA;        ///< 10-sample Simple Moving Average of lightLevel

/** Binary semaphore that guards lightLevel and SMA. */
SemaphoreHandle_t sema;

/** LCD object using I2C address 0x27, configured for a 16-column × 2-row display. */
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ================== Function Prototypes ==================
void lightDetectorTask(void *arg);
void lcdTask(void *arg);
void anomalyAlarmTask(void *arg);
void primeCalculationTask(void *arg);
int  calculateAvg(int buffer[]);
bool isPrime(int n);

// ================== Function Implementations ==================

/**
 * @brief FreeRTOS task (Core 0) that reads the photoresistor every 500 ms.
 * * Stores the value in a 10-element circular buffer, and updates the
 * global lightLevel and SMA under semaphore protection.
 * * @param arg Unused task parameter (required by FreeRTOS API)
 * @return void (FreeRTOS tasks never return)
 */
void lightDetectorTask(void *arg) {
  pinMode(PHOTO_PIN, INPUT);
  int buffer[10] = {0}; 
  int index = 0;

  while (1) {
    int currentLight = analogRead(PHOTO_PIN);
    if (xSemaphoreTake(sema, portMAX_DELAY) == pdTRUE) {
      lightLevel = currentLight;
      buffer[index] = lightLevel;
      index = (index + 1) % 10; 
      SMA = calculateAvg(buffer);
      xSemaphoreGive(sema); 
    }
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

/**
 * @brief Computes the arithmetic mean of a 10-element integer array.
 * * Used by lightDetectorTask to produce the Simple Moving Average.
 * * @param buffer Pointer to an array of exactly 10 integer ADC samples
 * @return int The integer average (truncated, not rounded) of the 10 values
 */
int calculateAvg(int buffer[]) {
  int sum = 0;
  for (int i = 0; i < 10; i++) {
    sum += buffer[i];
  }
  return sum / 10;
}

/**
 * @brief FreeRTOS task (Core 0) that polls the shared lightLevel and SMA.
 * * Refreshes the I2C LCD every 250 ms only when either value has changed 
 * since the last update, avoiding unnecessary redraws.
 * * @param arg Unused task parameter (required by FreeRTOS API)
 * @return void (FreeRTOS tasks never return)
 */
void lcdTask(void *arg) {
  int lastLight = -1;
  int lastSMA = -1;

  while (1) {
    int currentLight = 0;
    int currentSMA = 0;

    if (xSemaphoreTake(sema, portMAX_DELAY) == pdTRUE) {
      currentLight = lightLevel;
      currentSMA = SMA;
      xSemaphoreGive(sema);
    }

    if (currentLight != lastLight || currentSMA != lastSMA) {
      lcd.setCursor(0, 0);
      lcd.print("Light: ");
      lcd.print(currentLight);
      lcd.print("    "); 
      
      lcd.setCursor(0, 1);
      lcd.print("SMA: ");
      lcd.print(currentSMA);
      lcd.print("    ");

      lastLight = currentLight;
      lastSMA = currentSMA;
    }
    
    vTaskDelay(250 / portTICK_PERIOD_MS);
  }
}

/**
 * @brief FreeRTOS task (Core 1) that monitors SMA and flashes the alarm LED.
 * * Flashes the LED 3 times in ~1 second whenever the SMA falls outside the 
 * normal range (300–3800). A 2-second pause follows each burst.
 * * @param arg Unused task parameter (required by FreeRTOS API)
 * @return void (FreeRTOS tasks never return)
 */
void anomalyAlarmTask(void *arg) {
  pinMode(LED_PIN, OUTPUT);

  while (1) {
    int currentSMA = 0;

    if (xSemaphoreTake(sema, portMAX_DELAY) == pdTRUE) {
      currentSMA = SMA;
      xSemaphoreGive(sema);
    }

    if (currentSMA > 3800 || currentSMA < 300) {
      for(int i = 0; i < 3; i++) {
        digitalWrite(LED_PIN, HIGH);
        vTaskDelay(166 / portTICK_PERIOD_MS); 
        digitalWrite(LED_PIN, LOW);
        vTaskDelay(166 / portTICK_PERIOD_MS);
      }
      vTaskDelay(2000 / portTICK_PERIOD_MS);
    } else {
      vTaskDelay(250 / portTICK_PERIOD_MS);
    }
  }
}

/**
 * @brief FreeRTOS task (Core 1) that prints prime numbers up to 5000.
 * * Operates independently of the semaphore. Yields 1 ms per iteration to 
 * prevent starving the FreeRTOS IDLE task.
 * * @param arg Unused task parameter (required by FreeRTOS API)
 * @return void (FreeRTOS tasks never return)
 */
void primeCalculationTask(void *arg) {
  while(1) {
    for (int n = 2; n <= 5000; n++) {
      if (isPrime(n)) {
        Serial.printf("%d ", n);
      }
      vTaskDelay(1 / portTICK_PERIOD_MS);
    }
    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }
}

/**
 * @brief Determines whether a given integer is a prime number.
 * * @param n The integer to test (values <= 1 always return false)
 * @return bool True if n is prime, false otherwise
 */
bool isPrime(int n) {
    if (n <= 1) return false;
    for (int i = 2; i * i <= n; i++) {
        if (n % i == 0) return false;
    }
    return true;
}

/**
 * @brief Standard Arduino setup function to initialize hardware, semaphores, and tasks.
 */
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

  xTaskCreatePinnedToCore(lightDetectorTask, "Light Detector Task", 2048, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(lcdTask, "LCD Task", 2048, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(anomalyAlarmTask, "Anomaly Alarm Task", 2048, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(primeCalculationTask, "Prime Calculation Task", 2048, NULL, 1, NULL, 1);
}

/**
 * @brief Standard Arduino loop function (Empty because FreeRTOS is handling execution).
 */
void loop() {}