#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "nrf.h"
#include "nrf_delay.h"
#include "microbit_v2.h"
#include "app_timer.h"

#include "seg7.h"

NRF_TWI_MNGR_DEF(twi_mngr_instance, 1, 0);
static bool debug = true; 

int main(void) {
    if (debug) printf("Board started ...\n");
    // i2c config & initialization
    nrf_drv_twi_config_t i2c_config = NRF_DRV_TWI_DEFAULT_CONFIG;
    i2c_config.scl = EDGE_P19;
    i2c_config.sda = EDGE_P20;
    i2c_config.frequency = NRF_DRV_TWI_FREQ_100K;
    i2c_config.interrupt_priority = 0;
    nrf_twi_mngr_init(&twi_mngr_instance, &i2c_config);
    
    app_timer_init();
    seg7_init(&twi_mngr_instance, 20, debug);
    nrf_delay_ms(3000); 

    int it = 0;
    seg7_sub_sec(5);
    while(1) {
        printf("Countdown: %d\n", seg7_get_countdown()); 
        if (time_ran_out()) {
            printf("\nBOOM\n");
            break;
        }
        if (seg7_get_countdown() == 5) {
            seg7_stop_timer();
            nrf_delay_ms(2000);
            seg7_start_timer();
        }
        nrf_delay_ms(1000);
        it++;
    }
}