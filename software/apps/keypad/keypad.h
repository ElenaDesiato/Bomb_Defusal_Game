#ifndef KEYPAD_H
#define KEYPAD_H
#include <stdint.h>

#define NUM_ROWS 4
#define NUM_COLS 3

bool keypad_init(uint8_t i2c_addr, const uint8_t* row_pins, const uint8_t* col_pins);

void keypad_clear_input_record(void);
void keypad_start_scanning(void);
void keypad_stop_scanning(void);

char* keypad_get_input(void); 
uint8_t keypad_get_input_length(void); 

void print_keypad_input(void);

#endif