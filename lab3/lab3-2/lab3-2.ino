#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ==========================================
// 1. PIN DEFINITIONS
// ==========================================
#define LED_PIN       4
#define BUZZER_PIN    5
#define OSCOPE_PIN    6

// ==========================================
// 2. SCHEDULER & TCB DEFINITIONS
// ==========================================
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

// ==========================================
// 3. TASK CONTEXT VARIABLES
// ==========================================
// These global variables let Tasks B, C, and D remember where they 
// are between scheduler calls, and allows Task E to reset them.
int b_count = 1, b_runs = 0;
int c_note_idx = 0, c_runs = 0;
char d_letter = 'A'; int d_runs = 0;

void reset_task_contexts() {
  b_count = 1; b_runs = 0;
  c_note_idx = 0; c_runs = 0;
  d_letter = 'A'; d_runs = 0;
  // v3.0 syntax for setting duty cycle to 0 (silence)
  ledcWrite(BUZZER_PIN, 0); 
}

// ==========================================
// 4. REQUIRED OS FUNCTIONS
// ==========================================
void my_sleep(unsigned long n_msec) {
  tasks[current_task].state = SLEEPING;
  tasks[current_task].wake_time = millis() + n_msec;
}

void halt_me() {
  tasks[current_task].state = HALTED;
  Serial.print("Completed: ");
  Serial.print(tasks[current_task].name);
  Serial.print(": Priority ");
  Serial.println(tasks[current_task].priority);
}

// ==========================================
// 5. TASK FUNCTIONS
// ==========================================

// --- Task A: LED Blinker ---
// Blink 8 times a sec (8Hz). 1 cycle = 125ms -> Toggle every 62ms
void task_a_blink() {
  static bool led_state = false;
  led_state = !led_state;
  digitalWrite(LED_PIN, led_state ? HIGH : LOW);
  my_sleep(62);
}

// --- Task B: LCD Counter ---
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

// --- Task C: Music Player ---
// --- Task C: Music Player ---
const int notes[10] = {261, 293, 329, 349, 392, 440, 493, 523, 587, 659};
void task_c_music() {
  if (c_runs >= 2) {
    ledcWrite(BUZZER_PIN, 0); // Stop PWM sound
    halt_me();
    return;
  }

  int freq = notes[c_note_idx];
  
  // v3.0 syntax: Change frequency on the fly
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

// --- Task D: Alphabet Printer ---
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

// --- Task E: Priority Updater ---
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

// --- Task F: Extra Credit O-Scope ---
void task_f_oscope() {
  static bool pin_state = false;
  pin_state = !pin_state;
  digitalWrite(OSCOPE_PIN, pin_state ? HIGH : LOW);
  
  // Sleep for 1ms. By asking the scheduler to wake this up every 1ms,
  // we force a predictable 1ms scheduling "tick" visible on the scope.
  my_sleep(1); 
}

// ==========================================
// 6. SETUP
// ==========================================
void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  pinMode(OSCOPE_PIN, OUTPUT);
  
  // v3.0 syntax: Attach pin, set frequency, and set resolution all in one step
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

// ==========================================
// 7. THE SCHEDULER LOOP
// ==========================================
void loop() {
  unsigned long current_millis = millis();

  // 1. Wake up sleeping tasks
  for (int i = 0; i < NUM_TASKS; i++) {
    if (tasks[i].state == SLEEPING && current_millis >= tasks[i].wake_time) {
      tasks[i].state = READY;
    }
  }

  // 2. Find the READY task with the highest priority (lowest number)
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

  // 3. Execute the chosen task
  if (best_task != -1) {
    current_task = best_task;
    tasks[best_task].state = RUNNING;
    
    tasks[best_task].func(); // <--- Run the Task!

    // If the task didn't update its own state to SLEEPING or HALTED, return to READY
    if (tasks[current_task].state == RUNNING) {
      tasks[current_task].state = READY;
    }
    
    last_task = best_task; // Update Round-Robin tracker
  }
}