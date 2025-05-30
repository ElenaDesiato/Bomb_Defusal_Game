#pragma once

#define NUM_SWITCHES 5 

void switch_puzzle_init(uint16_t puzzle_reset_pin, uint16_t* puzzle_switch_pins, uint16_t puzzle_neopixel_pin);
// Start puzzle. Return true iff puzzle is complete.
bool switch_puzzle_start(void); 
// Returns true iff puzzle has been successfully completed.
bool switch_puzzle_is_complete(void);