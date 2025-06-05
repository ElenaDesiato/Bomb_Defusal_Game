#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "nrf.h"
#include "nrf_delay.h"
#include "microbit_v2.h"
#include "app_timer.h"

#include "../switch_puzzle/switch_puzzle.h"
#include "../morse_puzzle/morse_puzzle.h"
#include "../sx1509/sx1509.h"
#include "../DFR0760/DFR0760.h"

NRF_TWI_MNGR_DEF(twi_mngr_instance, 1, 0);
static bool debug = true; 

int main(void) {
    if (debug) printf("Board started ...\n");
    // i2c config & initialization
    nrf_drv_twi_config_t i2c_config = NRF_DRV_TWI_DEFAULT_CONFIG;
    i2c_config.scl = EDGE_P19;
    i2c_config.sda = EDGE_P20;
    i2c_config.frequency = NRF_DRV_TWI_FREQ_100K;
    i2c_config.interrupt_priority = 0;
    nrf_twi_mngr_init(&twi_mngr_instance, &i2c_config);

    uint8_t i2c_addr0 = SX1509_ADDR_00; 
    uint8_t i2c_addr1 = SX1509_ADDR_10; 

    // Initialize gpio expanders
    sx1509_init(i2c_addr0, &twi_mngr_instance); 
    sx1509_init(i2c_addr1, &twi_mngr_instance); 

    // Initialize App Timer
    app_timer_init();

    // Initialize switch puzzle
    switch_puzzle_pins_t switch_puzzle_pins = {
        // All on breakout
        .switches = {EDGE_P3, EDGE_P4, EDGE_P5, EDGE_P6, EDGE_P7},
        .puzzle_select = EDGE_P14,
        .neopixel = EDGE_P9,
    };
    switch_puzzle_init(&switch_puzzle_pins, debug); 


    // Initialize morse code puzzle
    const morse_puzzle_pins_t morse_puzzle_pins = {
        // first few on gpio 10 expander (addr1)
        .rows = {0, 1, 2, 3},
        .cols = {4, 5, 6},
        .puzzle_select = 15,
        .led_g = 12,
        .led_b = 13,
        .led_r = 14,
        .buzzer = EDGE_P13 // on breakout
    };
    morse_puzzle_init(i2c_addr1,&twi_mngr_instance,&morse_puzzle_pins, debug); 

    // Configure main button 
    uint8_t main_start_button = 8; 
    sx1509_pin_config_input_pullup(i2c_addr1, main_start_button); 
    // puzzle select buttons configured as inputs in puzzles themselves

    // Initialize TTS
    DFR0760_init(&twi_mngr_instance); 
    DFR0760_set_volume(4);

    // Main game logic
    bool is_game_running = false; 
    while (1) {
        // Start timer
        if(!is_game_running && !sx1509_pin_read(i2c_addr1,main_start_button)) {
            if (debug) printf("Timer started.\n"); 
            is_game_running = true; 
            morse_puzzle_start();
            switch_puzzle_start(); 
            // TO DO: start timer
        }
        // Morse Puzzle
        
        if(is_game_running && !morse_puzzle_is_complete() && !sx1509_pin_read(i2c_addr1,morse_puzzle_pins.puzzle_select)) {
            if (debug) printf("Morse puzzle started \n");
            switch_puzzle_stop();
            morse_puzzle_continue(NULL); 
        }

        // Switch Puzzle
        if(is_game_running && !switch_puzzle_is_complete() && !nrf_gpio_pin_read(switch_puzzle_pins.puzzle_select)){
            if (debug) printf("Switch puzzle started \n"); 
            morse_puzzle_stop();
            switch_puzzle_continue(NULL); 
        }

        // Game successfully completed
        if (is_game_running && morse_puzzle_is_complete() && switch_puzzle_is_complete()) {
            if (debug) printf("Game successfully completed!\n"); 
            is_game_running = false; 
            DFR0760_say("Congratulations!"); 
        }
        nrf_delay_ms(100); 
    }
}
