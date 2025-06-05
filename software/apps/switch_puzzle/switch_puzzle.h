#pragma once

#define NUM_SWITCHES 5 

typedef struct {
    uint8_t switches[NUM_SWITCHES];     
    uint8_t puzzle_select;
    uint8_t neopixel;
} switch_puzzle_pins_t; 

// Initialize puzzle. Only call once.
void switch_puzzle_init(switch_puzzle_pins_t* pins, bool debug);

// Start puzzle (reset puzzle state & generate new solution)
void switch_puzzle_start(void); 

// Continue playing existing puzzle instance
void switch_puzzle_continue(void* _unused); 

// End everything related to the puzzle.
void switch_puzzle_stop(void); 

// Returns true iff puzzle has been successfully completed.
bool switch_puzzle_is_complete(void);