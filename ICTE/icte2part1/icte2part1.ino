#include "driver/gpio.h"            // For GPIO driver-level macros
#include "soc/io_mux_reg.h"         // For configuring the MUX (multiplexer)
#include "soc/gpio_reg.h"           // For direct GPIO register access
#include "soc/gpio_periph.h"        // For GPIO pin definitions


// --- Constants --- //
#define GPIO_PIN 5    // GPIO pin connected to the LED


void setup() {
  // --- TODO: Set the pin function to GPIO using the appropriate macro --- //
  // Hint: Use a macro to select the GPIO function for GPIO_PIN
  PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[GPIO_PIN], PIN_FUNC_GPIO);
  


  // --- TODO: Enable the GPIO pin as an output --- //
  // Hint: Use GPIO_ENABLE_REG and a bitwise OR operation
  *((volatile uint32_t *)GPIO_ENABLE_REG) |= (1 << GPIO_PIN);
}


void loop() {
  // --- TODO: Turn the LED ON --- //
  // Hint: Set the output register bit corresponding to GPIO_PIN to 1
  *((volatile uint32_t *)GPIO_OUT_REG) |= (1 << GPIO_PIN);


  // Wait for 1 second
  delay(1000);


  // --- TODO: Turn the LED OFF --- //
  // Hint: Clear the output register bit corresponding to GPIO_PIN
  *((volatile uint32_t *)GPIO_OUT_REG) &= ~(1 << GPIO_PIN);


  // Wait again
  delay(1000);
}
