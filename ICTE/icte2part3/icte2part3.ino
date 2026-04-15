#include "driver/gpio.h"
#include "soc/io_mux_reg.h"
#include "soc/gpio_reg.h"
#include "soc/gpio_periph.h"
#include "soc/timer_group_reg.h"


// TODO: Define GPIO pin number
#define GPIO_PIN 5


// TODO: Define toggle interval in timer ticks (e.g., 1 second)
#define LED_TOGGLE_INTERVAL 1000


// TODO: Define TIMER_INCREMENT_MODE and TIMER_ENABLE macros
#define TIMER_INCREMENT_MODE (1 << 30)
#define TIMER_ENABLE (1 << 31)

#define TIMERG0_474BASE     0x6001F000


#define TIMG0_474T0CONFIG   ((volatile uint32_t *)(TIMERG0_474BASE + 0))
#define TIMG0_474T0LO       ((volatile uint32_t *)(TIMERG0_474BASE + 4))
#define TIMG0_474T0HI       ((volatile uint32_t *)(TIMERG0_474BASE + 8))
#define TIMG0_474T0UPDATE   ((volatile uint32_t *)(TIMERG0_474BASE + 12))

void setup() {
  // TODO: Set GPIO_PIN function to GPIO using MUX macro
  PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[GPIO_PIN], PIN_FUNC_GPIO);


  // TODO: Enable GPIO_PIN as output
  *((volatile uint32_t *)GPIO_ENABLE_REG) |= (1 << GPIO_PIN);



  // TODO: Configure timer
  uint32_t timer_config = 0;


  // TODO: Optionally apply a clock divider
  timer_config |= (80 << 13);


  // TODO: Set increment mode and enable timer
  timer_config |= TIMER_INCREMENT_MODE | TIMER_ENABLE;


  // TODO: Write config to timer register
  *((volatile uint32_t *)TIMG0_474T0CONFIG) = timer_config;


  // TODO: Trigger a timer update to load settings
  *((volatile uint32_t *)TIMG0_474T0UPDATE) = 1;
}


void loop() {
  // Track last toggle time
  static uint32_t last_toggle_time = 0;


  // TODO: Read current timer value
  uint32_t current_time = 0;
  current_time = *((volatile uint32_t *)TIMG0_474T0LO);


  // TODO: Check if toggle interval has passed
  if ((current_time - last_toggle_time) >= LED_TOGGLE_INTERVAL) {
    // TODO: Read current GPIO output state
    uint32_t gpio_out = 0;
    gpio_out = *((volatile uint32_t *)GPIO_OUT_REG);


    // TODO: Toggle GPIO_PIN using XOR
    *((volatile uint32_t *)GPIO_OUT_REG) = gpio_out ^ (1 << GPIO_PIN);


    // TODO: Update last_toggle_time
    last_toggle_time = current_time;
  }


  // TODO: Refresh timer counter value
  *((volatile uint32_t *)GPIO_OUT_REG) = 1;
}

