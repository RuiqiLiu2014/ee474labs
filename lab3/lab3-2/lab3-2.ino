// Filename: lab3-2.ino
// Authors: Ruiqi Liu, Hailey Yuan
// Date: 5/8/2026
// Description: This file uses a scheduler to perform a variety of tasks, including blinking an LED 
// and playing different notes on the buzzer.
// Gemini-909

// INCLUDES

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// MACROS

#define LED_PIN       4
#define BUZZER_PIN    5
#define OSCOPE_PIN    6

// ENUMS, STRUCTS, AND GLOBAL VARIABLES

enum TaskState { RUNNING, READY, HALTED, SLEEPING };

typedef void (*TaskFunction)(void);

// Task Control Block structure
typedef struct {
  const char* name;
  TaskFunction func;
  uint8_t priority;       // 0 is highest, 7 is lowest
  TaskState state;
  unsigned long wake_time; // Time in millis() when sleep ends
} TCB;

#define NUM_TASKS 6
TCB tasks[NUM_TASKS];
int current_task = -1; // Tracks which task is currently executing

LiquidCrystal_I2C lcd(0x27, 16, 2);

// These global variables let Tasks B, C, and D remember where they 
// are between scheduler calls, and allows Task E to reset them.
int b_count = 1, b_runs = 0;
int c_note_idx = 0, c_runs = 0;
char d_letter = 'A'; int d_runs = 0;

// FUNCTIONS

// Name: reset_task_contexts
// Description: resets all tasks to their initial state
void reset_task_contexts() {
  b_count = 1; b_runs = 0;
  c_note_idx = 0; c_runs = 0;
  d_letter = 'A'; d_runs = 0;
  ledcWrite(BUZZER_PIN, 0); 
}

// Name: my_sleep
// Description: sets task state to sleeping and determines the time when the task should wake up
void my_sleep(unsigned long n_msec) {
  tasks[current_task].state = SLEEPING;
  tasks[current_task].wake_time = millis() + n_msec;
}

// Name: halt_me
// Description: sets task to the halted state
void halt_me() {
  tasks[current_task].state = HALTED;
  Serial.print("Completed: ");
  Serial.print(tasks[current_task].name);
  Serial.print(": Priority ");
  Serial.println(tasks[current_task].priority);
}

// Name: task_a_blink
// Description: Blinks an LED at a frequency of 8 Hz.
void task_a_blink() {
  static bool led_state = false;
  led_state = !led_state;
  digitalWrite(LED_PIN, led_state ? HIGH : LOW);
  my_sleep(62);
}

// Name: task_b_counter
// Description: Counts from 1-10 on the LCD at a rate of one number every two seconds
void task_b_counter() {
  if (b_runs >= 2) {
    halt_me();
    return;
  }
  lcd.setCursor(0, 0);
  lcd.print("Count: ");
  lcd.print(b_count);
  lcd.print("    "); // Clear trailing chars

  b_count++;
  if (b_count > 10) {
    b_count = 1;
    b_runs++;
  }
  my_sleep(2000); // 1 count every 2 seconds
}

// Name: task_c_music
// Description: Plays a sequence of 10 notes on the buzzer
const int notes[10] = {261, 293, 329, 349, 392, 440, 493, 523, 587, 659};
void task_c_music() {
  if (c_runs >= 2) {
    ledcWrite(BUZZER_PIN, 0); // Stop PWM sound
    halt_me();
    return;
  }

  int freq = notes[c_note_idx];

  ledcChangeFrequency(BUZZER_PIN, freq, 8);
  ledcWrite(BUZZER_PIN, 128); // 50% duty cycle (Volume)

  lcd.setCursor(0, 1);
  lcd.print("Note: "); lcd.print(freq); lcd.print(" Hz  ");

  c_note_idx++;
  if (c_note_idx >= 10) {
    c_note_idx = 0;
    c_runs++;
  }
  my_sleep(500); // Play note for 500ms
}

// Name: task_d_alphabet
// Description: Displays the alphabet to the serial monitor at a rate of 2 letters per second.
void task_d_alphabet() {
  if (d_runs >= 2) {
    halt_me();
    return;
  }
  
  Serial.print(d_letter);
  Serial.print(" ");
  
  d_letter++;
  if (d_letter > 'Z') {
    d_letter = 'A';
    d_runs++;
    Serial.println();
  }
  my_sleep(500); // 2 letters per sec = 500ms sleep
}

// Name: task_e_updater
// Description: Updates the priority scheme the scheduler runs.
void task_e_updater() {
  static bool first_run = true;
  // Let it sleep 30s immediately on boot before switching to Scheme 2
  if (first_run) {
    first_run = false;
    my_sleep(30000);
    return; 
  }

  static int scheme = 1;
  scheme++;
  if (scheme > 3) scheme = 1;

  Serial.print("\n>>> Switching to Priority Scheme ");
  Serial.println(scheme);

  if (scheme == 1) {
    tasks[0].priority = 2; tasks[1].priority = 3; tasks[2].priority = 4; tasks[3].priority = 5;
  } else if (scheme == 2) {
    tasks[0].priority = 5; tasks[1].priority = 4; tasks[2].priority = 2; tasks[3].priority = 3;
  } else if (scheme == 3) {
    tasks[0].priority = 2; tasks[1].priority = 2; tasks[2].priority = 2; tasks[3].priority = 2;
  }

  reset_task_contexts();

  // Re-enable Tasks A-D
  for (int i = 0; i < 4; i++) {
    tasks[i].state = READY;
  }

  my_sleep(30000); // Sleep 30 seconds until next update
}

// Name: task_f_oscope
// Description: Sends pulses to the oscilloscope pin.
void task_f_oscope() {
  static bool pin_state = false;
  pin_state = !pin_state;
  digitalWrite(OSCOPE_PIN, pin_state ? HIGH : LOW);
  
  // Sleep for 1ms. By asking the scheduler to wake this up every 1ms,
  // we force a predictable 1ms scheduling "tick" visible on the scope.
  my_sleep(1); 
}

// SETUP AND LOOP

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  pinMode(OSCOPE_PIN, OUTPUT);
  
  ledcAttach(BUZZER_PIN, 2000, 8); 

  lcd.init();
  lcd.backlight();

  // Initialize TCB Array for Scheme 1
  tasks[0] = {"LED Blinker",  task_a_blink,    2, READY, 0};
  tasks[1] = {"LCD Counter",  task_b_counter,  3, READY, 0};
  tasks[2] = {"Music Player", task_c_music,    4, READY, 0};
  tasks[3] = {"Alpha Print",  task_d_alphabet, 5, READY, 0};
  tasks[4] = {"Updater",      task_e_updater,  1, READY, 0}; 
  tasks[5] = {"O-Scope",      task_f_oscope,   0, READY, 0}; 

  lcd.print("Priority Scheme 1");
  Serial.println("Priority Scheme 1");
  delay(2000); 
  lcd.clear();
}

void loop() {
  unsigned long current_millis = millis();

  // Wake up sleeping tasks
  for (int i = 0; i < NUM_TASKS; i++) {
    if (tasks[i].state == SLEEPING && current_millis >= tasks[i].wake_time) {
      tasks[i].state = READY;
    }
  }

  // Find the READY task with the highest priority (lowest number)
  int best_task = -1;
  uint8_t best_priority = 255; 
  static int last_task = 0; // Remembers last task for Round-Robin

  for (int i = 0; i < NUM_TASKS; i++) {
    // Check array circularly to ensure Round-Robin for equal priorities
    int check_idx = (last_task + 1 + i) % NUM_TASKS;
    
    if (tasks[check_idx].state == READY) {
      if (tasks[check_idx].priority < best_priority) {
        best_priority = tasks[check_idx].priority;
        best_task = check_idx;
      }
    }
  }

  // Execute the chosen task
  if (best_task != -1) {
    current_task = best_task;
    tasks[best_task].state = RUNNING;
    
    tasks[best_task].func(); // Run the task

    // If the task didn't update its own state to SLEEPING or HALTED, return to READY
    if (tasks[current_task].state == RUNNING) {
      tasks[current_task].state = READY;
    }
    
    last_task = best_task; // Update Round-Robin tracker
  }
}