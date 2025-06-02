#include "microbit_v2.h"
#include "nrf_delay.h"
#include "app_timer.h"
#include "morse_puzzle.h"
#include "../sx1509/sx1509.h"

NRF_TWI_MNGR_DEF(twi_mngr_instance, 1, 0);

int main(void) {
  printf("Board started ...\n");
  nrf_drv_twi_config_t i2c_config = NRF_DRV_TWI_DEFAULT_CONFIG;
  i2c_config.scl = EDGE_P19;
  i2c_config.sda = EDGE_P20;
  i2c_config.frequency = NRF_DRV_TWI_FREQ_100K;
  i2c_config.interrupt_priority = 0;
  nrf_twi_mngr_init(&twi_mngr_instance, &i2c_config);
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
  uint8_t i2c_addr = SX1509_ADDR_10; 
  app_timer_init();
  morse_puzzle_init(i2c_addr, &twi_mngr_instance, &pins,true); 

  while (1) {
    if (!morse_puzzle_is_complete() && !sx1509_pin_read(i2c_addr,pins.puzzle_select)){
      morse_puzzle_start();
    }
    nrf_delay_ms(100);
  }
}