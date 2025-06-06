#pragma once

#include "nrf_twi_mngr.h"

#define NUM_SWITCHES 5

typedef struct {
    uint8_t rows[4];      
    uint8_t cols[3];      
    uint8_t puzzle_select;
    uint8_t led_g;
    uint8_t led_b;
    uint8_t led_r;
    uint8_t buzzer;
} morse_puzzle_pins_t; 

// Initialize puzzle; only call once
void morse_puzzle_init(uint8_t i2c_addr, const nrf_twi_mngr_t* twi_mgr_instance, const morse_puzzle_pins_t* pins, bool debug);

// Start puzzle (reset state & generate new solution)
void morse_puzzle_start(void); 

// Continue playing existing puzzle instance
void morse_puzzle_continue(void* _unused); 

// End everything related to the puzzle
void morse_puzzle_stop(void);

// Returns true iff puzzle has been successfully completed.
bool morse_puzzle_is_complete(void);

// Change color of puzzle LED
void morse_set_LED_green(void);
void morse_set_LED_red(void);
void morse_set_LED_white(void);
void morse_set_LED_off(void);