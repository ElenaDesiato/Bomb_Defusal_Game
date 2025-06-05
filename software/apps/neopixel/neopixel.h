#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef struct {
  uint8_t r;
  uint8_t g;
  uint8_t b;
} color_t;

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

void neopixel_init(void);
void neopixel_set_rgb(neopixel_device_t device, uint8_t index, uint8_t r, uint8_t g, uint8_t b);
void neopixel_set_color(neopixel_device_t device, uint8_t led_index, color_name_t color_name);
void neopixel_set_color_all(neopixel_device_t device, color_name_t color_name);
void neopixel_clear(neopixel_device_t device, uint8_t led_index);
void neopixel_clear_all(neopixel_device_t device);
void neopixel_show(neopixel_device_t device);
