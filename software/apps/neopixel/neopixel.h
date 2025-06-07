#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef struct {
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t w; 
} color_t;

typedef struct {
  uint8_t ring;
  uint8_t jewel; 
  uint8_t stick; 
} neopixel_pins_t; 

typedef enum {
  NEO_RING,
  NEO_JEWEL,
  NEO_STICK,
  NEO_DEV_COUNT // Total number of Neopixel devices
} neopixel_device_t;

typedef enum {
  COLOR_BLACK,
  COLOR_WHITE,
  COLOR_RED,
  COLOR_GREEN,
  COLOR_BLUE,
  COLOR_YELLOW,
  COLOR_CYAN,
  COLOR_MAGENTA,
  COLOR_COUNT
} color_name_t;

// Initialize all neopixel devices at once
void neopixel_init(const neopixel_pins_t* pins, bool debug);

// Set color of neopixel devices
void neopixel_set_rgbw(neopixel_device_t device, uint8_t index, uint8_t w, uint8_t r, uint8_t g, uint8_t b);
void neopixel_set_color(neopixel_device_t device, uint8_t led_index, color_name_t color_name);
void neopixel_set_color_all(neopixel_device_t device, color_name_t color_name);
// Turn off neopixel devices
void neopixel_clear(neopixel_device_t device, uint8_t led_index);
void neopixel_clear_all(neopixel_device_t device);

void print_combined_sequence(int length);
