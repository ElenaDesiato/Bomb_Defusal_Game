#ifndef KEYPAD_H
#define KEYPAD_H
#include <stdint.h>

#define NUM_ROWS 4
#define NUM_COLS 3

bool keypad_init(void);

void keypad_clear_input_record(void);
void keypad_stop_scanning(void);

char* keypad_get_input(void); 
uint8_t keypad_get_input_length(void); 

void print_keypad_input(void);

#endif