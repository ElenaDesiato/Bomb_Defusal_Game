#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>

#include "nrf_delay.h"

#include "microbit_v2.h"
#include "neopixel.h"
#include "nrf_delay.h"

int main(void) {
  printf("Board started!\n");
  neopixel_init();  
  neopixel_clear_all(NEO_RING);

  while (1) {
    for(int i = 0; i < 3; i++) {
      neopixel_set_color(NEO_RING, i, COLOR_RED);
      neopixel_show(NEO_RING);
      nrf_delay_ms(100);
    }
    for(int i = 0; i < 3; i++) {
      neopixel_clear(NEO_RING, i);
      neopixel_show(NEO_RING);
      nrf_delay_ms(100);
    }
  }
}