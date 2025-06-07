#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>

#include "nrf_delay.h"

#include "microbit_v2.h"
#include "neopixel.h"
#include "nrf_delay.h"

static const neopixel_pins_t neopixel_pins = {
  .ring = EDGE_P9,
  .jewel = EDGE_P2,
  .stick = EDGE_P1,
};

int main(void) {
  printf("Board started!\n");
  neopixel_init(&neopixel_pins,false);  
  neopixel_clear_all(NEO_RING);
  neopixel_clear_all(NEO_JEWEL);
  neopixel_clear_all(NEO_STICK);
  nrf_delay_ms(100);

  while (1) {
    for(int i = 0; i < 16; i++) {
      neopixel_set_color(NEO_RING, i, COLOR_RED);
      neopixel_set_color(NEO_STICK, i, COLOR_GREEN);
      neopixel_set_color(NEO_JEWEL, i, COLOR_BLUE);
      nrf_delay_ms(100);
    }
    for(int i = 0; i < 16; i++) {
      neopixel_clear(NEO_RING, i);
      neopixel_clear(NEO_STICK, i);
      neopixel_clear(NEO_JEWEL, i);
      nrf_delay_ms(100);
    }
    //print_combined_sequence(24*16);
  }
}