#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "nrf_twi_mngr.h"

typedef struct {
    uint8_t puzzle_select; // pin 7 on ADDR00
    uint8_t neopixel_stick; // ?
} accel_puzzle_pins_t; 

typedef struct {
    int pitch;
    int roll;
} accel_puzzle_instruction_t; 

void accel_puzzle_init(uint8_t i2c_addr, const nrf_twi_mngr_t* twi_mgr_instance, accel_puzzle_pins_t* p_pins, bool enable_debug);
bool accel_puzzle_start(void);
void accel_puzzle_stop(void);
void accel_puzzle_continue(void* _unused);
bool is_accel_puzzle_active(void);
bool is_accel_puzzle_complete(void);