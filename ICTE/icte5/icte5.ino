void toggleLED1(void *p);
void toggleLED2(void *p);

#define STATE_RUNNING   0
#define STATE_READY     1
#define STATE_WAITING   2
#define STATE_INACTIVE  3
#define STATE_SLEEP     4

// TCB Struct
typedef struct {
  void (*fptr)(); // function pointer
  void *arg_ptr; // argument pointer
  unsigned short int state; // task state
  unsigned long delay; // sleeeeeeep delay
  unsigned long interval; // time between toggling
} TCBStruct;

// LED Pins
const int ledPin1 = 37;
const int ledPin2 = 38;

// Time of last update
unsigned long lastTime37 = 0;
unsigned long lastTime38 = 0;

// LED state
bool state37 = LOW;
bool state38 = LOW;

TCBStruct TaskList[2];

void setup() {
  // put your setup code here, to run once:
  pinMode(ledPin1, OUTPUT);
  pinMode(ledPin2, OUTPUT);

  // Assign TCB struct values
  TaskList[0].fptr = toggleLED1;
  TaskList[0].arg_ptr = NULL;
  TaskList[0].state = STATE_READY;
  TaskList[0].delay = 0;
  TaskList[0].interval = 1000;
  TaskList[1].fptr = toggleLED2;
  TaskList[1].arg_ptr = NULL;
  TaskList[1].state = STATE_READY;
  TaskList[1].delay = 0;
  TaskList[1].interval = 1500;
}

void loop() {
  unsigned long now = millis();
  for (int i = 0; i < 2; i++) {
    // Wakes up task and sets state to ready
    if (TaskList[i].state == STATE_SLEEP && now >= TaskList[i].delay) {
      TaskList[i].state = STATE_READY;    // wake up
    }

    // Runs task after seeing ready state
    if (TaskList[i].state == STATE_READY) {
      // Task called sets state back to sleep when done
      TaskList[i].fptr();
    }
  }
}

// Name: toggleLED1
// Description: changes state of first LED
void toggleLED1() {
  state37 = !state37;
  digitalWrite(ledPin1, state37);
  sleep(0, TaskList[0].interval);
}

// Name: toggleLED2
// Description: changes state of second LED
void toggleLED2() {
  state38 = !state38;
  digitalWrite(ledPin2, state38);
  sleep(1, TaskList[1].interval);
}

// Name: sleep
// Description: changes task state to sleep
void sleep(int taskIndex, unsigned long d_ms) {
  TaskList[taskIndex].state = STATE_SLEEP;
  TaskList[taskIndex].delay = millis() + d_ms;   // absolute wake time
}

// void toggleLED(int pin, unsigned long interval, unsigned long &lastTime, bool &state) {
//   unsigned long currentTime = millis();
//   if (currentTime - lastTime >= interval) {
//     state = !state; 
//     digitalWrite(pin, state);
//     lastTime = currentTime;
//   }
// }