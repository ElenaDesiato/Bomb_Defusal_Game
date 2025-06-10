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

// Initialize puzzle. Only call once.
void accel_puzzle_init(uint8_t i2c_addr, const nrf_twi_mngr_t* twi_mgr_instance, const accel_puzzle_pins_t* p_pins, bool enable_debug);

// Start puzzle (reset puzzle state)
bool accel_puzzle_start(void);

// End everything related to the puzzle.
void accel_puzzle_stop(void);

// Continue playing existing puzzle instance
void accel_puzzle_continue(void* _unused);

// Returns true iff puzzle is running.
bool accel_puzzle_is_active(void);

// Returns true iff puzzle has been successfully completed.
bool accel_puzzle_is_complete(void);