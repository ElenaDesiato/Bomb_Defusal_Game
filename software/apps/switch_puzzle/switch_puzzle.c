#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#include "app_timer.h"
#include "nrf_delay.h"
#include "microbit_v2.h"
#include "../neopixel/neopixel.h"
#include "rng.h"

#include "switch_puzzle.h"

// Configuration
APP_TIMER_DEF(loop_timer);
static uint8_t switch_pins[NUM_SWITCHES];
static uint8_t select_pin = 0; 
// Solution of puzzle
static bool switch_pins_state[NUM_SWITCHES] = {0,0,0,0,0};
// mapping of switches to values (each value is 16 bits corresponding to LEDs)
static uint16_t switch_pins_mapped_values[NUM_SWITCHES] = {0,0,0,0,0};
// Current sum of mapped values of switches that are in the correct position (solution = 0xFFFF)
static uint16_t sum = 0;

static bool debug = false; 

// Make randomized mapping for the switches
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
        printf("SWITCH: Mapped Values:\n");
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

// Helper function: Read all switches & update current sum by comparing them to expected states (in switch_pins_state)
static void update_switch_status() {
    sum = 0;
    for (int i = 0; i < NUM_SWITCHES; i++) {
        if (nrf_gpio_pin_read(switch_pins[i]) == switch_pins_state[i]) {
            sum += switch_pins_mapped_values[i];
        }
    }
}

// Helper function: Update neopixel
static void update_neopixel_ring() {   
    for(int i = 0; i < 16; i++) {
        if ((sum >> i) & 1) {
            neopixel_set_color(NEO_RING, i, COLOR_RED);
        } else {
            neopixel_clear(NEO_RING,i);
        }
    }
}

// Return true iff puzzle is complete
bool switch_puzzle_is_complete() {
    return sum == 0xFFFF; 
}

// Initialize the puzzle (Update global variables, iniitalize drivers & pins, get random seed)
// Must be called first and should only be called once
void switch_puzzle_init(const switch_puzzle_pins_t* pins, bool p_debug) {
    debug = p_debug; 
    select_pin = pins->puzzle_select; 
    for (int i = 0; i < NUM_SWITCHES; i++) {
        switch_pins[i] = pins->switches[i];
    }
    nrf_gpio_cfg_input(select_pin, NRF_GPIO_PIN_PULLUP);
    for (int i = 0; i < NUM_SWITCHES; i++) {
        nrf_gpio_cfg_input(switch_pins[i], NRF_GPIO_PIN_NOPULL);
    }
    rng_init(); 

    // random generator seed
    srand(rng_get8());
    app_timer_create(&loop_timer, APP_TIMER_MODE_SINGLE_SHOT, switch_puzzle_continue);
    sum = 0;
}

// Start one iteration of the puzzle (Reset & generate new random mapping); Only call after init
void switch_puzzle_start(void) {
    if (debug) printf("SWITCH: Puzzle started.\n");
    sum = 0; 
    update_neopixel_ring(); 
    randomize_switch_mapping();
}

// Continue playing existing iteration of puzzle. Only call after start.
void switch_puzzle_continue(void* _unused) {    
    update_switch_status();
    if (switch_puzzle_is_complete()){
        neopixel_set_color_all(NEO_RING,COLOR_GREEN);
        if (debug) printf("SWITCH: Success!.\n");
        switch_puzzle_stop();
    }
    else {
        update_neopixel_ring();
        app_timer_start(loop_timer, APP_TIMER_TICKS(10), NULL);
    }
}

// Stop everything related to the switch puzzle
void switch_puzzle_stop(void) {
    if (debug) printf("SWITCH: Puzzle stopped.\n");
    app_timer_stop(loop_timer);
    if (!switch_puzzle_is_complete()) neopixel_clear_all(NEO_RING);
}

