#pragma once

#include <stdint.h>
#include <stdbool.h>


typedef struct {
    uint8_t puzzle_select;
    uint8_t neopixel_stick;
} accel_puzzle_pins_t; 

typedef struct {
    int pitch;
    int roll;
} accel_puzzle_instruction_t; 

void accel_puzzle_init(uint8_t sx1509_addr, accel_puzzle_pins_t* p_pins, bool enable_debug);
bool accel_puzzle_start(void);

bool is_accel_puzzle_active(void);