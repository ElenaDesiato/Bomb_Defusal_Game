#include "microbit_v2.h"
#include "nrf_delay.h"
#include "morse_puzzle.h"
#include "../sx1509/sx1509.h"

int main(void) {
  morse_puzzle_pins_t pins = {
    // first few on gpio expander
    .rows = {0, 1, 2, 3},
    .cols = {4, 5, 6},
    .puzzle_select = 15,
    .led_g = 12,
    .led_b = 13,
    .led_r = 14,
    .buzzer = EDGE_P13 // on breakout
  };
  morse_puzzle_init(SX1509_ADDR_10,&pins); 

  while (1) {
    if (morse_puzzle_start()) {
        printf("Morse puzzle complete!\n");
    }
    nrf_delay_ms(100);
  }
}