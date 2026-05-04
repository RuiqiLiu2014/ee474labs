void toggleLED1(void *p);
void toggleLED2(void *p);

#define STATE_RUNNING   0
#define STATE_READY     1
#define STATE_WAITING   2
#define STATE_INACTIVE  3

typedef struct TCBStruct {
  void (*ftpr)(void *p); // function pointer
  void *arg_ptr; // argument pointer
  unsigned short int state; // task state
  unsigned int delay; // sleeeeeeep delay
} TCBStruct;

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
  
  
  TaskList[0].fptr = toggleLED1;
  TaskList[0].arg_ptr = NULL;
  TaskList[0].state = STATE_READY;
  TaskList[0].delay = 0;
  TaskList[1].fptr = toggleLED2;
  TaskList[1].arg_ptr = NULL;
  TaskList[1].state = STATE_READY;
  TaskList[1].delay = 0;
}

void loop() {
  // put your main code here, to run repeatedly:
  for (int i = 0; i < 2; i++) {
    if (TaskList[i].state == STATE_READY) {
      TaskList[i].fptr(TaskList[i].arg_ptr);   // dispatch the task
    }
  }
}

void toggleLED1(void *p) {
  unsigned long currentTime = millis();
  if (currentTime - lastTime37 >= 1000) {
    state37 = !state37;
    digitalWrite(ledPin1, state37);
    lastTime37 = currentTime;
  }
}

void toggleLED2(void *p) {
  unsigned long currentTime = millis();
  if (currentTime - lastTime38 >= 2000) {
    state38 = !state38;
    digitalWrite(ledPin2, state38);
    lastTime38 = currentTime;
  }
}

// void toggleLED(int pin, unsigned long interval, unsigned long &lastTime, bool &state) {
//   unsigned long currentTime = millis();
//   if (currentTime - lastTime >= interval) {
//     state = !state; 
//     digitalWrite(pin, state);
//     lastTime = currentTime;
//   }
// }