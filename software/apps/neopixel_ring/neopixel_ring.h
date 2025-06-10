#ifndef NEOPIXEL_RING_H
#define NEOPIXEL_RING_H

#include <stdint.h>

// OUTDATED VERSION: See neopixel.c/neopixel.h for generalized driver 
// (that can also handle stick & jewel in addition to ring)

typedef struct {
  uint8_t r;
  uint8_t g;
  uint8_t b;
} color_t;

typedef enum {
  COLOR_BLACK,
  COLOR_WHITE,
  COLOR_RED,
  COLOR_GREEN,
  COLOR_BLUE,
  COLOR_YELLOW,
  COLOR_CYAN,
  COLOR_MAGENTA,
  COLOR_ORANGE,
  COLOR_PURPLE,
  COLOR_PINK,
  COLOR_COUNT
} color_name_t;

// Initialize neopixel ring, MUST be called first
void neopixel_ring_init(uint16_t neopixel_pin);

// Set LEDs to specified colors or RGB values
void neopixel_set_rgb(uint8_t led, uint8_t r, uint8_t g, uint8_t b);
void neopixel_set_color(uint8_t led, color_name_t color);
void neopixel_set_color_all(color_name_t color);

// Turn off Neopixel Ring LEDs
void neopixel_clear(uint8_t led);
void neopixel_clear_all(void);
#endif