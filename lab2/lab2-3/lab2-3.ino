#include "driver/gpio.h"
#include "soc/io_mux_reg.h"
#include "soc/gpio_reg.h"
#include "soc/gpio_periph.h"

#define TIMG0_BASE 0x6001F000
#define TIMG_T0CONFIG_ADDR (TIMG0_BASE + 0x0000)
#define TIMG_T0CONFIG_REG (*(volatile uint32_t *)(TIMG_T0CONFIG_ADDR))

// ESP32-S3 TRM: TIMG_T0LO_REG offset is 0x0004
#define TIMG_T0LO_ADDR (TIMG0_BASE + 0x0004)
#define TIMG_T0LO_REG (*(volatile uint32_t *)(TIMG_T0LO_ADDR))

// ESP32-S3 TRM: TIMG_T0UPDATE_REG offset is 0x000C
#define TIMG_T0UPDATE_ADDR (TIMG0_BASE + 0x000C)
#define TIMG_T0UPDATE_REG (*(volatile uint32_t *)(TIMG_T0UPDATE_ADDR))

const int testPin = 5; // Using GPIO 5 for the LED output

void setup() {
    // Configure GPIO 2 as an output pin
    PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[testPin], PIN_FUNC_GPIO);
    *(volatile uint32_t *)GPIO_ENABLE_REG |= (1 << testPin);

    // Configure Timer 0 in Timer Group 0
    
    // 1. Clear the prescaler divider bits (bits 13 to 28)
    TIMG_T0CONFIG_REG &= ~(0xFFFF << 13);
    
    // 2. Set the prescaler to 80 (80 MHz APB_CLK / 80 = 1 MHz -> 1 us per tick)
    TIMG_T0CONFIG_REG |= (80 << 13);
    
    // 3. Set the increase bit (bit 30) so the timer counts up
    TIMG_T0CONFIG_REG |= (1 << 30);
    
    // 4. Enable auto-reload (bit 29)
    TIMG_T0CONFIG_REG |= (1 << 29);
    
    // 5. Enable the timer (bit 31). Using (uint32_t) prevents signed bit shift warnings.
    TIMG_T0CONFIG_REG |= ((uint32_t)1 << 31);
}

void delayTimer(uint32_t delay_us) {
    // Trigger an update to copy the current running timer value to TIMG_T0LO_REG
    TIMG_T0UPDATE_REG |= 1; 
    uint32_t start_time = TIMG_T0LO_REG;

    while (true) {
        // Continuously trigger updates to get the most recent timer value
        TIMG_T0UPDATE_REG |= 1;
        if ((TIMG_T0LO_REG - start_time) >= delay_us) {
            break; // Exit the loop once the desired elapsed time is reached
        }
    }
}

void loop() {
    *(volatile uint32_t *)GPIO_OUT_REG |= (1 << testPin);  // Turn LED ON
    delayTimer(1000000);                                   // Delay 1 second (1,000,000 us)

    *(volatile uint32_t *)GPIO_OUT_REG &= ~(1 << testPin); // Turn LED OFF
    delayTimer(1000000);                                   // Delay 1 second
}