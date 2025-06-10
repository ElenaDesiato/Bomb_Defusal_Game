#pragma once

#include <stdint.h>

#define NUM_ROWS 4
#define NUM_COLS 3

// Initialize keypad module. Only call once at boot.
bool keypad_init(uint8_t i2c_addr, const uint8_t* row_pins, const uint8_t* col_pins);

// clear the input record
void keypad_clear_input_record(void);

// Start keypad scanning. Call this to start reading input.
void keypad_start_scanning(void);

// Stop keypad scanning. Call this to stop reading input.
void keypad_stop_scanning(void);

// return current input
char* keypad_get_input(void); 

// returns current input length
uint8_t keypad_get_input_length(void); 

//helper function to print current input recorded
void print_keypad_input(void);