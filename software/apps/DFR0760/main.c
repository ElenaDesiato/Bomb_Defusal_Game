#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h> 

#include "nrf_delay.h"
#include "nrf_twi_mngr.h" 
#include "nrf_drv_twi.h" 

#include "microbit_v2.h" 
#include "DFR0760.h" 

NRF_TWI_MNGR_DEF(twi_mngr_instance, 5, 0);

int main(void) {
    printf("Board started!\n");

    // I2C Init
    nrf_drv_twi_config_t i2c_config = NRF_DRV_TWI_DEFAULT_CONFIG;
    i2c_config.scl = EDGE_P19;
    i2c_config.sda = EDGE_P20;
    i2c_config.frequency = NRF_DRV_TWI_FREQ_100K;
    i2c_config.interrupt_priority = APP_IRQ_PRIORITY_LOW;
    ret_code_t err_code = nrf_twi_mngr_init(&twi_mngr_instance, &i2c_config);
    if (err_code != NRF_SUCCESS) {
        printf("I2C manager init failed! Error: 0x%lX\n", (unsigned long)err_code);
        while(1);
    }

    // DFR0760 Init
    DFR0760_init(&twi_mngr_instance);

    if (DFR0760_is_connected(&twi_mngr_instance)){
       printf("DFR0760 is connected.\n");
    } else {
       printf("DFR0760 is NOT connected.\n");
    }

    nrf_delay_ms(1000); 

    DFR0760_set_volume(1); // volume 1 is still super loud wtf

    DFR0760_say("testing ");
    nrf_delay_ms(500);

    while (1) {
        nrf_delay_ms(10000); // Idle
    }
}