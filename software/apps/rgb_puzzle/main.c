#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "nrf.h"
#include "nrf_delay.h"
#include "app_timer.h"
#include "nrf_twi_mngr.h"

#include "microbit_v2.h"
#include "sx1509.h"
#include "neopixel.h"
#include "rgb_puzzle.h"

NRF_TWI_MNGR_DEF(twi_mngr_instance, 5, 0);

// I2C Config
static const nrf_drv_twi_config_t i2c_config = {
    .scl = EDGE_P19,
    .sda = EDGE_P20,
    .frequency = NRF_DRV_TWI_FREQ_100K,
    .interrupt_priority = 0,
    .clear_bus_init = false
};

static const uint8_t gpio_i2c_addr0 = SX1509_ADDR_00; 

static const rgb_puzzle_pins_t rgb_puzzle_pins = {
    .red_button = 1,
    .green_button = 4,
    .blue_button = 3,
    .yellow_button = 2,// all on sx1509 00
    .puzzle_select = EDGE_P8, // on breakout
};

static const neopixel_pins_t neopixel_pins = {
  .ring = EDGE_P9,
  .jewel = EDGE_P2,
  .stick = EDGE_P1,
};

int main(void) {
  printf("Board started...\n");

  bool debug = true;
  app_timer_init();
  nrf_twi_mngr_init(&twi_mngr_instance, &i2c_config);

  sx1509_init(gpio_i2c_addr0, &twi_mngr_instance);

  neopixel_init(&neopixel_pins, debug);
  neopixel_clear_all(NEO_RING);
  neopixel_clear_all(NEO_JEWEL);
  neopixel_clear_all(NEO_STICK);

  rgb_puzzle_init(SX1509_ADDR_00, &rgb_puzzle_pins, debug);
  rgb_puzzle_start();


  while (1) {
    if (rgb_puzzle_is_complete()) {break;}

    if (!rgb_puzzle_is_complete() && !nrf_gpio_pin_read(rgb_puzzle_pins.puzzle_select)) {
      if (debug) printf("puzzle sel read: %d\n",  nrf_gpio_pin_read(rgb_puzzle_pins.puzzle_select));
      rgb_puzzle_continue(NULL);
    }
    nrf_delay_ms(100); 
  }
}