#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>

#include "nrf_delay.h"

#include "microbit_v2.h"
#include "neopixel_ring.h"
#include "nrf_delay.h"

int main(void) {
  printf("Board started!\n");
  neopixel_ring_init(EDGE_P9);  

  while (1) {
    for(int i = 0; i < 16; i++) {
      neopixel_set_color(i,COLOR_RED);
      nrf_delay_ms(100);
    }
    for(int i = 0; i < 16; i++) {
      neopixel_clear(i);
      nrf_delay_ms(100);
    }
  }
}