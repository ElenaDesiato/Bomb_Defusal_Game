#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "nrf.h"
#include "nrf_delay.h"
#include "app_timer.h"
#include "nrf_twi_mngr.h"

#include "microbit_v2.h"
#include "sx1509.h"
#include "DFR0760.h" 
#include "accel_puzzle.h"
#include "../neopixel/neopixel.h"

// I2C Manager instance
NRF_TWI_MNGR_DEF(twi_mngr_instance, 1, 0);

// I2C Configuration
static const nrf_drv_twi_config_t i2c_config = {
    .scl = EDGE_P19,
    .sda = EDGE_P20,
    .frequency = NRF_DRV_TWI_FREQ_100K,
    .interrupt_priority = 0,
    .clear_bus_init = false
};

// GPIO Expander Address
static const uint8_t gpio_i2c_addr0 = SX1509_ADDR_00; 
static const uint8_t gpio_i2c_addr1 = SX1509_ADDR_10; 

int main(void) {
  printf("Board started ...\n");

  // Define pins for the puzzle
  accel_puzzle_pins_t accel_puzzle_pins = {
    .puzzle_select = 7,         // Pin 7 on the SX1509
    .neopixel_stick = EDGE_P12  // Not used in this logic, but defined
  };

  app_timer_init();
  nrf_twi_mngr_init(&twi_mngr_instance, &i2c_config);
  DFR0760_init(&twi_mngr_instance);
  DFR0760_set_volume(9);
  sx1509_init(gpio_i2c_addr0, &twi_mngr_instance); 
  sx1509_init(gpio_i2c_addr1, &twi_mngr_instance); 
  neopixel_init(); 
  neopixel_clear_all(NEO_RING);
  neopixel_clear_all(NEO_JEWEL);
  neopixel_clear_all(NEO_STICK);

  bool debug = true;
  accel_puzzle_init(gpio_i2c_addr0, &twi_mngr_instance, &accel_puzzle_pins, debug); 
  accel_puzzle_start(); 

  while (1) {
    if (is_accel_puzzle_complete()) {
      if (debug) printf("ACCEL: Puzzle complete.\n");
      break;
    }

    if (!is_accel_puzzle_complete() && !sx1509_pin_read(gpio_i2c_addr0, accel_puzzle_pins.puzzle_select)) {
      accel_puzzle_continue(NULL);
    }
    nrf_delay_ms(100);
  }

}