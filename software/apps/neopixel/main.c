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
  neopixel_clear_all(NEO_JEWEL);
  neopixel_clear_all(NEO_STICK);
  nrf_delay_ms(100);

  while (1) {
    for(int i = 0; i < 16; i++) {
      neopixel_set_color(NEO_RING, i, COLOR_RED);
      neopixel_set_color(NEO_STICK, i, COLOR_GREEN);
      nrf_delay_ms(100);
    }
    for (int i = 0; i < 8; i++) {
      neopixel_set_color(NEO_JEWEL, i, COLOR_RED);
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