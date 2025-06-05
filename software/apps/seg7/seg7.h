#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "nrf_twi_mngr.h"

#define SEG7_I2C_ADDR 0x71

// Initialize timer & set starting countdown value to start_val. Does not start timer.
void seg7_init(const nrf_twi_mngr_t* i2c, uint32_t start_val, bool debug_mode);

// Set timer countdown value, can be used both when timer is running and when it is not
void seg7_set_countdown(uint32_t val);

// Start timer, by default will continue counting down from previous countdown
void seg7_start_timer(void); 
void seg7_stop_timer(void); 

// Modify timer values (subtract & add)
void seg7_add_sec(uint32_t delta);
void seg7_sub_sec(uint32_t delta);

uint32_t seg7_get_countdown(void);
bool time_ran_out(void);
