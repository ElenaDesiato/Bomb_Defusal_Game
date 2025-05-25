#include <stdbool.h>
#include <stdint.h>
#include <stdio.h> 
#include "nrf_delay.h"
#include "microbit_v2.h"
#include "LSM6DSO.h"

NRF_TWI_MNGR_DEF(twi_mngr_instance, 1, 0);

static void tilt_callback(lsm6dso_measurement_t m, void* _unused) {
  printf("Tilt angle: X: %.2f, Y: %.2f, Z: %.2f\n", m.x_axis, m.y_axis, m.z_axis); 
}

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
    lsm6dso_get_tilt_nonblocking(tilt_callback, NULL); 
    printf("This might print before the tilt as its nonblocking\n"); 
    nrf_delay_ms(1000);
  }
}