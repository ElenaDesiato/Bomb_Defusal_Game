#pragma once

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

void morse_puzzle_init(uint8_t i2c_addr, morse_puzzle_pins_t* pins);
// Start puzzle. Return true iff puzzle is complete.
bool morse_puzzle_start(void); 
// Returns true iff puzzle has been successfully completed.
bool morse_puzzle_is_complete(void);