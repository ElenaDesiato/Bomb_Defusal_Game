#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "nrf.h"
#include "nrf_delay.h"
#include "microbit_v2.h"
#include "app_timer.h"

#include "../switch_puzzle/switch_puzzle.h"
#include "../morse_puzzle/morse_puzzle.h"
#include "../accel_puzzle/accel_puzzle.h"
#include "../rgb_puzzle/rgb_puzzle.h"
#include "../sx1509/sx1509.h"
#include "../DFR0760/DFR0760.h"
#include "../seg7/seg7.h"
#include "../neopixel/neopixel.h"

static bool debug = true; 

// Game parameters
#define GAME_LENGTH_SEC 5*60
#define TTS_VOLUME_LEVEL 1

// Various configuration parameters
NRF_TWI_MNGR_DEF(twi_mngr_instance, 32, 0);

static const nrf_drv_twi_config_t i2c_config = {
    .scl = EDGE_P19,
    .sda = EDGE_P20,
    .frequency = NRF_DRV_TWI_FREQ_100K,
    .interrupt_priority = 0,
    .clear_bus_init = false
};
static const uint8_t gpio_i2c_addr0 = SX1509_ADDR_00; 
static const uint8_t gpio_i2c_addr1 = SX1509_ADDR_10; 

static const neopixel_pins_t neopixel_pins = {
  .ring = EDGE_P9,
  .jewel = EDGE_P2,
  .stick = EDGE_P1,
};

static const switch_puzzle_pins_t switch_puzzle_pins = {
    // All on breakout
    .switches = {EDGE_P3, EDGE_P4, EDGE_P5, EDGE_P6, EDGE_P7},
    .puzzle_select = EDGE_P14,
};

static const morse_puzzle_pins_t morse_puzzle_pins = {
    // first few on 10 gpio expander (addr1)
    .rows = {0, 1, 2, 3},
    .cols = {4, 5, 6},
    .puzzle_select = 15,
    .led_g = 12,
    .led_b = 13,
    .led_r = 14,
    .buzzer = EDGE_P13 // on breakout
};

static const accel_puzzle_pins_t accel_puzzle_pins = {
    .puzzle_select = EDGE_P0,         // Pin 0 on breakout
    .neopixel_stick = EDGE_P1 
};

static const rgb_puzzle_pins_t rgb_puzzle_pins = {
    // all on breakout
    .red_button = EDGE_P15,     // was 1
    .green_button = EDGE_P10, // was 4
    .blue_button = EDGE_P11,   // was 3
    .yellow_button = EDGE_P12,        // 2
    .puzzle_select = EDGE_P8, 
};

#define START_BUTTON_PIN 8 // on 10 gpio expander

// Helper functions to take care of basic game logic (starting game, completing game, running out of time)
static void start_game(bool *is_game_running) {
    if (debug) printf("Timer started.\n"); 
    *is_game_running = true; 
    morse_puzzle_start();
    switch_puzzle_start(); 
    seg7_set_countdown(GAME_LENGTH_SEC); 
    seg7_start_timer();  
}

static void handle_out_of_time(bool *is_game_running) {
    if (debug) printf("Time ran out.\n"); 
    *is_game_running = false; 
    seg7_stop_timer(); 
    neopixel_set_color_all(NEO_RING, COLOR_RED);
    neopixel_set_color_all(NEO_STICK, COLOR_RED);
    neopixel_set_color_all(NEO_JEWEL, COLOR_RED);
    morse_set_LED_red();
    DFR0760_say("Boom"); 
}

static void complete_game(bool *is_game_running) {
    if (debug) printf("Game successfully completed!\n"); 
    *is_game_running = false; 
    seg7_stop_timer(); 
    neopixel_set_color_all(NEO_RING, COLOR_GREEN);
    neopixel_set_color_all(NEO_STICK, COLOR_GREEN);
    neopixel_set_color_all(NEO_JEWEL, COLOR_GREEN);
    morse_set_LED_green();
    DFR0760_say("Congratulations"); 
}

int main(void) {
    if (debug) printf("Board started ...\n");

    // Initialize App Timer library
    app_timer_init();

    // i2c initialization
    nrf_twi_mngr_init(&twi_mngr_instance, &i2c_config);

    // Initialize gpio expanders
    sx1509_init(gpio_i2c_addr0, &twi_mngr_instance); 
    sx1509_init(gpio_i2c_addr1, &twi_mngr_instance); 

    // Initialize neopixel separately as it is used in multiple puzzles
    neopixel_init(&neopixel_pins, debug);

    // Initialize timer module
    seg7_init(&twi_mngr_instance, GAME_LENGTH_SEC, debug); 

    // Initialize switch puzzle
    switch_puzzle_init(&switch_puzzle_pins, debug); 

    // Initialize morse code puzzle
    morse_puzzle_init(gpio_i2c_addr1,&twi_mngr_instance,&morse_puzzle_pins, debug); 

    // Initialize accelerometer puzzle
    accel_puzzle_init(gpio_i2c_addr0, &twi_mngr_instance,&accel_puzzle_pins, debug); 

    // Initialize rgb puzzle
    rgb_puzzle_init(gpio_i2c_addr0, &rgb_puzzle_pins, debug); 

    // Configure main button 
    sx1509_pin_config_input_pullup(gpio_i2c_addr1, START_BUTTON_PIN); 
    // puzzle select buttons configured as inputs in puzzles themselves

    // Initialize TTS
    DFR0760_init(&twi_mngr_instance); 
    DFR0760_set_volume(TTS_VOLUME_LEVEL);

    // Main game logic
    bool is_game_running = false; 
    while (1) {
        // Start timer
        if(!is_game_running && !sx1509_pin_read(gpio_i2c_addr1,START_BUTTON_PIN)) {
            start_game(&is_game_running); 
        }

        // Morse Puzzle
        if(is_game_running && !morse_puzzle_is_complete() && !sx1509_pin_read(gpio_i2c_addr1,morse_puzzle_pins.puzzle_select)) {
            if (debug) printf("Morse puzzle started \n");
            switch_puzzle_stop();
            accel_puzzle_stop();
            rgb_puzzle_stop();
            morse_puzzle_continue(NULL); 
        }

        // Switch Puzzle
        if(is_game_running && !switch_puzzle_is_complete() && !nrf_gpio_pin_read(switch_puzzle_pins.puzzle_select)){
            if (debug) printf("Switch puzzle started \n"); 
            morse_puzzle_stop();
            accel_puzzle_stop();
            rgb_puzzle_stop();
            switch_puzzle_continue(NULL); 
        }

        // Accelerometer Puzzle
        if(is_game_running && !accel_puzzle_is_complete() && !nrf_gpio_pin_read(accel_puzzle_pins.puzzle_select)){
            if (debug) printf("Accelerometer puzzle started \n"); 
            morse_puzzle_stop();
            switch_puzzle_stop();
            rgb_puzzle_stop();
            accel_puzzle_continue(NULL); 
        }

        // RGB Puzzle
        if(is_game_running && !rgb_puzzle_is_complete() && !nrf_gpio_pin_read(rgb_puzzle_pins.puzzle_select)){
            if (debug) printf("RGB puzzle started \n"); 
            morse_puzzle_stop();
            switch_puzzle_stop();
            accel_puzzle_stop();
            rgb_puzzle_continue(NULL); 
        }

        // Game successfully completed
        if (is_game_running && morse_puzzle_is_complete() && switch_puzzle_is_complete() && accel_puzzle_is_complete() && rgb_puzzle_is_complete()) {
            complete_game(&is_game_running); 
        }

        // Timer ran out 
        if (is_game_running && time_ran_out() && (!morse_puzzle_is_complete() || !switch_puzzle_is_complete() || !accel_puzzle_is_complete() || !rgb_puzzle_is_complete())) {
            handle_out_of_time(&is_game_running);
        }
        nrf_delay_ms(100); 
    }
}
