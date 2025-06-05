#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#include "app_timer.h"
#include "nrf_delay.h"
#include "microbit_v2.h"
#include "../neopixel_ring/neopixel_ring.h"
#include "rng.h"

#include "switch_puzzle.h"

static uint8_t switch_pins[NUM_SWITCHES] = {0, 0, 0, 0, 0};
static bool switch_pins_state[NUM_SWITCHES] = {0,0,0,0,0};
static uint8_t select_pin = 0; 
static uint16_t sum = 0;
static uint16_t switch_pins_mapped_values[NUM_SWITCHES] = {0,0,0,0,0};

static bool debug = false; 

APP_TIMER_DEF(loop_timer);

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
    return sum == 0xFFFF; 
}

void switch_puzzle_init(switch_puzzle_pins_t* pins, bool p_debug) {
    debug = p_debug; 
    select_pin = pins->puzzle_select; 
    for (int i = 0; i < NUM_SWITCHES; i++) {
        switch_pins[i] = pins->switches[i];
    }
    neopixel_ring_init(pins->neopixel);  
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

void switch_puzzle_start(void) {
    sum = 0; 
    update_neopixel_ring(); 
    randomize_switch_mapping();
}

void switch_puzzle_continue(void* _unused) {    
    update_switch_status();
    if (switch_puzzle_is_complete()){
        neopixel_set_color_all(COLOR_GREEN);
        if (debug) printf("Switch puzzle: Success!.\n");
        switch_puzzle_stop();
    }
    else {
        update_neopixel_ring();
        app_timer_start(loop_timer, APP_TIMER_TICKS(10), NULL);
    }
}


void switch_puzzle_stop(void) {
    app_timer_stop(loop_timer);
    if (!switch_puzzle_is_complete()) neopixel_clear_all();
}

