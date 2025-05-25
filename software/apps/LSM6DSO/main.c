#include <stdbool.h>
#include <stdint.h>
#include <stdio.h> 
#include "nrf_delay.h"
#include "microbit_v2.h"
#include "LSM6DSO.h"

NRF_TWI_MNGR_DEF(twi_mngr_instance, 1, 0);

int main(void) {
  printf("Board started!\n");

  // Initialize I2C peripheral and driver
  nrf_drv_twi_config_t i2c_config = NRF_DRV_TWI_DEFAULT_CONFIG;
  i2c_config.scl = EDGE_P19;
  i2c_config.sda = EDGE_P20;
  i2c_config.frequency = NRF_TWIM_FREQ_100K;
  i2c_config.interrupt_priority = 0;
  nrf_twi_mngr_init(&twi_mngr_instance, &i2c_config);

  // Initialize the LSM303AGR accelerometer/magnetometer sensor
  lsm6dso_init(&twi_mngr_instance);

  // Loop forever
  while (1) {
    if (lsm6dso_is_ready()) {
      float curr = lsm6dso_get_tilt(); 
      printf("Tilt angle: %.2f \n", curr); 
    }
    nrf_delay_ms(1000);
  }
}