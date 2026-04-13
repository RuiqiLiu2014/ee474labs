#include "driver/gpio.h"
#include "soc/io_mux_reg.h"
#include "soc/gpio_reg.h"
#include "soc/gpio_periph.h"
#include "soc/timer_group_reg.h"


// TODO: Define GPIO pin number
#define GPIO_PIN 5


// TODO: Define toggle interval in timer ticks (e.g., 1 second)
#define LED_TOGGLE_INTERVAL ________


// TODO: Define TIMER_INCREMENT_MODE and TIMER_ENABLE macros
// #define TIMER_INCREMENT_MODE ...
// #define TIMER_ENABLE ...


void setup() {
  // TODO: Set GPIO_PIN function to GPIO using MUX macro
  PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[GPIO_PIN], PIN_FUNC_GPIO);


  // TODO: Enable GPIO_PIN as output
  *((volatile uint32_t *)GPIO_ENABLE_REG) |= (1 << GPIO_PIN);



  // TODO: Configure timer
  uint32_t timer_config = 0;


  // TODO: Optionally apply a clock divider
  // timer_config |= ...;


  // TODO: Set increment mode and enable timer
  // timer_config |= ...;


  // TODO: Write config to timer register
  // *((volatile uint32_t *)...) = timer_config;


  // TODO: Trigger a timer update to load settings
  // *((volatile uint32_t *)...) = 1;
}


void loop() {
  // Track last toggle time
  static uint32_t last_toggle_time = 0;


  // TODO: Read current timer value
  uint32_t current_time = 0;
  // current_time = *((volatile uint32_t *)...);


  // TODO: Check if toggle interval has passed
  if ((current_time - last_toggle_time) >= LED_TOGGLE_INTERVAL) {
    // TODO: Read current GPIO output state
    uint32_t gpio_out = 0;
    // gpio_out = *((volatile uint32_t *)...);


    // TODO: Toggle GPIO_PIN using XOR
    // *((volatile uint32_t *)...) = gpio_out ^ ...;


    // TODO: Update last_toggle_time
    last_toggle_time = current_time;
  }


  // TODO: Refresh timer counter value
  // *((volatile uint32_t *)...) = 1;
}

