#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>

#include "nrf_delay.h"
#include "nrf_twi_mngr.h"

#include "microbit_v2.h"
#include "sx1509.h"


NRF_TWI_MNGR_DEF(twi_mngr_instance, 1, 0);

int main(void) {
  printf("Board started!\n");

  // If you are using QWIIC or other external I2C devices, the are
  // connected to EDGE_P19 (a.k.a. I2C_QWIIC_SCL) and EDGE_P20 (a.k.a. I2C_QWIIC_SDA)
  nrf_drv_twi_config_t i2c_config = NRF_DRV_TWI_DEFAULT_CONFIG;
  i2c_config.scl = EDGE_P19;
  i2c_config.sda = EDGE_P20;
  i2c_config.frequency = NRF_DRV_TWI_FREQ_100K;
  i2c_config.interrupt_priority = 0;
  nrf_twi_mngr_init(&twi_mngr_instance, &i2c_config);

  //scan_i2c_bus(&twi_mngr_instance);

  sx1509_init(&twi_mngr_instance);
  printf("SX1509 is %s\n", sx1509_is_connected(&twi_mngr_instance) ? "connected" : "not connected");

  sx1509_pin_config_input_pullup(0); 
  
  while (1) {
    printf("Pin 0 is %s\n", sx1509_pin_read(0) ? "pressed" : "not pressed");
    nrf_delay_ms(100);
  }
}