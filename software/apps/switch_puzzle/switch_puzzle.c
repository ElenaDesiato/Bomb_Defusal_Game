#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#include "nrf_delay.h"
#include "microbit_v2.h"
#include "../neopixel_ring/neopixel_ring.h"
#include "rng.h"

#include "switch_puzzle.h"

static uint16_t switch_pins[NUM_SWITCHES] = {0, 0, 0, 0, 0};
static uint16_t switch_pins_mapped_values[NUM_SWITCHES] = {0,0,0,0,0};
static bool switch_pins_state[NUM_SWITCHES] = {0,0,0,0,0};
static uint16_t reset_pin = 0; 
static uint16_t sum = 0;

static bool debug = true; 

// Making randomized mapping for the switches
static void randomize_switch_mapping() {
    uint16_t target_sum = 0xFFFF; 
    uint16_t new_sum = 0;

    for (int i = 0; i < NUM_SWITCHES - 1; i++) {
        switch_pins_mapped_values[i] = rand() & 0xFFFF; 
        new_sum += switch_pins_mapped_values[i]; 
    }
    switch_pins_mapped_values[NUM_SWITCHES - 1] = target_sum - new_sum;

    for (int i = 0; i < NUM_SWITCHES; i++) {
        switch_pins_state[i] = rand() % 2;
    }

    if (debug) {
        printf("Mapped Values:\n");
        for (int i = 0; i < NUM_SWITCHES; i++) {
            printf("Switch %d: Mapped Value = %04X, State = %d\n", i, switch_pins_mapped_values[i], switch_pins_state[i]);
        }
    }

    // Verify OR result
    uint16_t or_result = 0;
    for (int i = 0; i < NUM_SWITCHES; i++) {
        or_result |= switch_pins_mapped_values[i];
    }
    if (debug) printf("Bitwise OR result: %04X\n", or_result);  // Should be FFFF

    if (or_result != 0xFFFF) {randomize_switch_mapping();} 
}

static void update_switch_status() {
    sum = 0;
    for (int i = 0; i < NUM_SWITCHES; i++) {
        if (nrf_gpio_pin_read(switch_pins[i]) == switch_pins_state[i]) {
            sum += switch_pins_mapped_values[i];
        }
    }
}

static void update_neopixel_ring() {   
    for(int i = 0; i < 16; i++) {
        if ((sum >> i) & 1) {
            neopixel_set_color(i, COLOR_RED);
        } else {
            neopixel_clear(i);
        }
    }
}

bool switch_puzzle_is_complete() {
    if (sum == 0xFFFF) {return true;}
    return false;
}

void switch_puzzle_init(uint16_t puzzle_reset_pin, uint16_t* puzzle_switch_pins, uint16_t puzzle_neopixel_pin) {
    reset_pin = puzzle_reset_pin;
    for (int i = 0; i < NUM_SWITCHES; i++) {
        switch_pins[i] = puzzle_switch_pins[i];
    }
    neopixel_ring_init(puzzle_neopixel_pin);  
    nrf_gpio_cfg_input(reset_pin, NRF_GPIO_PIN_PULLUP);
    for (int i = 0; i < NUM_SWITCHES; i++) {
        nrf_gpio_cfg_input(switch_pins[i], NRF_GPIO_PIN_NOPULL);
    }
    rng_init(); 

    // reset globals
    sum = 0; 
    for (int i = 0; i < NUM_SWITCHES; i++) {
        switch_pins_state[i] = 0;
        switch_pins_mapped_values[i] = 0;
    }

    // random generator seed
    srand(rng_get8());
}


bool switch_puzzle_start(void) {    
    if (debug) printf("Started switch puzzle.\n");
    randomize_switch_mapping();

    while(1) {
        // Reset if puzzle select is pressed
        if (!nrf_gpio_pin_read(reset_pin)) {
            printf("RESET button pressed. Puzzle reset.\n");
            return false; 
        }
    
        update_switch_status();
        update_neopixel_ring();
        if (switch_puzzle_is_complete()){
            neopixel_set_color_all(COLOR_GREEN);
            printf("Success!.\n");
            return true;
        }
        nrf_delay_ms(400);
    }
    return false; 
}

