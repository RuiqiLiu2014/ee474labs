/*
 ‘* Timer register definitions for ESP32-xx (change to your variant)
 ‘*
 */
#define TIMERG0_474BASE     0x6001F000


#define TIMG0_474T0CONFIG   ((volatile uint32_t *)(TIMERG0_474BASE + 0))
#define TIMG0_474T0LO       ((volatile uint32_t *)(TIMERG0_474BASE + 4))
#define TIMG0_474T0HI       ((volatile uint32_t *)(TIMERG0_474BASE + 8))
#define TIMG0_474T0UPDATE   ((volatile uint32_t *)(TIMERG0_474BASE + 12))
