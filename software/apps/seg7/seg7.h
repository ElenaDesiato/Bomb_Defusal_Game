#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "nrf_twi_mngr.h"

#define SEG7_I2C_ADDR 0x71

void seg7_init(const nrf_twi_mngr_t* i2c, int countdown_sec, bool debug_mode);

void seg7_set_countdown(uint32_t sec);

void seg7_add_sec(uint32_t delta);

void seg7_sub_sec(uint32_t delta);

uint32_t seg7_get_countdown(void);

bool time_runs_out(void);
