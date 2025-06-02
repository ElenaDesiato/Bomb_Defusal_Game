#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>

#include "nrf_delay.h"
#include "nrf_twi_mngr.h"

#include "microbit_v2.h"
#include "../sx1509/sx1509.h"

// keypad
const uint8_t row_pins[] = {0, 1, 2, 3};  // Pin 0 ~ Pin 3 on SX1509
const uint8_t col_pins[] = {4, 5, 6};   // Pin 4 ~ Pin 6 on SX1509
#define NUM_ROWS 4
#define NUM_COLS 3

const char key_map[4][3] = {
  {'1', '2', '3'},  
  {'4', '5', '6'}, 
  {'7', '8', '9'}, 
  {'*', '0', '#'} 
};


NRF_TWI_MNGR_DEF(twi_mngr_instance, 1, 0);

int main(void) {
  printf("Board started!\n");

  // i2c config & initialization
  nrf_drv_twi_config_t i2c_config = NRF_DRV_TWI_DEFAULT_CONFIG;
  i2c_config.scl = EDGE_P19;
  i2c_config.sda = EDGE_P20;
  i2c_config.frequency = NRF_DRV_TWI_FREQ_100K;
  i2c_config.interrupt_priority = 0;
  nrf_twi_mngr_init(&twi_mngr_instance, &i2c_config);
  uint8_t i2c_addr = SX1509_ADDR_10; 

  sx1509_init(i2c_addr, &twi_mngr_instance);

  // Row pins -> Output
  for (int i = 0; i < 4; i++) {
    sx1509_pin_config_output(i2c_addr, row_pins[i]);
    sx1509_pin_write(i2c_addr, row_pins[i], true); // set high at init
  }

  // Col pins -> Input (pull up)
  for (int i = 0; i < 3; i++) {
    sx1509_pin_config_input_pullup(i2c_addr, col_pins[i]);
  }

  char key_prev = '\0'; 
  while (1) {
    char key_current = '\0';

    for (int r = 0; r < 4; r++) {
      sx1509_pin_write(i2c_addr, row_pins[r], false);  // Scanned Row: Set Output as low
      nrf_delay_us(10);

      for (int c = 0; c < 3; c++) {
        if (!sx1509_pin_read(i2c_addr, col_pins[c])) { // Scan Cols: low -> col is pressed
          key_current = key_map[r][c];
          break; 
        }
      }
      sx1509_pin_write(i2c_addr, row_pins[r], true); // Write current row back to high

      if (key_current != '\0') { // If key already found, end row scanning early
          break;
      }
    }

    if (key_current != '\0') { // A key is currently being pressed in this scan
      if (key_current != key_prev) { // New key press event
        printf("Key pressed: %c\n", key_current);
        key_prev = key_current; // update key prev
      }
    } else { 
      if (key_prev != '\0') {
        printf("Key released: %c\n", key_prev);
        key_prev = '\0';
      }
    }
    nrf_delay_ms(20);
  }
}