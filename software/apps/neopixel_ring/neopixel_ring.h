#ifndef NEOPIXEL_RING_H
#define NEOPIXEL_RING_H

#include <stdint.h>

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

void neopixel_set_rgb(uint8_t led, uint8_t r, uint8_t g, uint8_t b);
void neopixel_ring_init(uint16_t neopixel_pin);
void neopixel_show(void);
void neopixel_clear(uint8_t led);
void neopixel_clear_all(void);
void neopixel_set_color(uint8_t led, color_name_t color);
void neopixel_set_color_all(color_name_t color);
#endif