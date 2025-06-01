#include "microbit_v2.h"
#include "nrf_delay.h"
#include "accel_puzzle.h"
#include "../sx1509/sx1509.h"

int main(void) {
  accel_puzzle_pins_t pins = {
    // first few on gpio expander
    .puzzle_select = 15,
    .neopixel_stick = EDGE_P12 // might change after we get our neopixel stick. now just use printf to debug.
  };
  bool debug_mode = true;
  accel_puzzle_init(SX1509_ADDR_10, &pins, debug_mode); 

  while (1) {
    if (accel_puzzle_start()) {
        printf("Accel puzzle complete!\n");
    }
    nrf_delay_ms(100);
  }
}