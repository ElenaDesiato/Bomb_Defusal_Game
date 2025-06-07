#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nrf_delay.h"
#include "nrf_twi_mngr.h"
#include "app_timer.h"

#include "microbit_v2.h"
#include "../lilybuzzer/lilybuzzer.h"
#include "../sx1509/sx1509.h"
#include "../keypad/keypad.h"

#include "morse_puzzle.h"
typedef enum {
    LED_OFF,
    LED_COLOR_WHITE,
    LED_COLOR_RED,
    LED_COLOR_GREEN
} led_color_t;

APP_TIMER_DEF(loop_timer);

static uint8_t sx1509_i2c_addr = 0; 
static const morse_puzzle_pins_t* pins = NULL; 

#define SOLUTION_LENGTH 4
static char solution_str[SOLUTION_LENGTH + 1];
//static char* solution_str = "";
static bool is_puzzle_complete = false; 

static bool debug = false; 

// Helper functions to turn led green, red, white respectively
void morse_set_LED_red() {
  sx1509_pin_clear(sx1509_i2c_addr,  pins->led_r); 
  sx1509_pin_set(sx1509_i2c_addr,  pins->led_g); 
  sx1509_pin_set(sx1509_i2c_addr,  pins->led_b); 
}
void morse_set_LED_green() {
  sx1509_pin_set(sx1509_i2c_addr,  pins->led_r); 
  sx1509_pin_clear(sx1509_i2c_addr,  pins->led_g); 
  sx1509_pin_set(sx1509_i2c_addr,  pins->led_b); 
}
void morse_set_LED_blue() {
  sx1509_pin_set(sx1509_i2c_addr,  pins->led_r); 
  sx1509_pin_set(sx1509_i2c_addr,  pins->led_g); 
  sx1509_pin_clear(sx1509_i2c_addr,  pins->led_b); 
}
void morse_set_LED_white() {
  sx1509_pin_clear(sx1509_i2c_addr,  pins->led_r); 
  sx1509_pin_clear(sx1509_i2c_addr,  pins->led_g); 
  sx1509_pin_clear(sx1509_i2c_addr,  pins->led_b); 
}  
void morse_set_LED_off() {
  sx1509_pin_set(sx1509_i2c_addr,  pins->led_r); 
  sx1509_pin_set(sx1509_i2c_addr,  pins->led_g); 
  sx1509_pin_set(sx1509_i2c_addr,  pins->led_b); 
}

// Helper function to check if keypad input matches solution

static bool check_sequence(void){
  //if (debug) {printf("MORSE: check_sequence called\n");}
  //print_keypad_input();
  char* input = keypad_get_input();
  volatile uint8_t input_len = keypad_get_input_length();
  if (input_len < SOLUTION_LENGTH) {
    //if (debug) {printf("MORSE: input length = %d\n", input_len);}
    if (debug && input_len != 0) printf("Input: %s, Input Length: %d\n", input, input_len);
    return false;
  }

  if (strcmp(input, solution_str) != 0){ // strings not equal
    morse_set_LED_red(); 
    keypad_clear_input_record();
    if (debug) printf("Morse puzzle: Failure\n");
    if (debug) printf("Input: %s, Input Length: %d\n", input, input_len);
    return false;
  }
  return true; 
}

// Helper function to generate a random solution
void morse_solution_gen(void);

void morse_puzzle_init(uint8_t i2c_addr, const nrf_twi_mngr_t* twi_mgr_instance, const morse_puzzle_pins_t* puzzle_pins, bool p_debug) {
  debug = p_debug;
  sx1509_i2c_addr = i2c_addr; 

  pins = puzzle_pins; 
  is_puzzle_complete = false; 

  keypad_init(sx1509_i2c_addr,pins->rows, pins->cols); 
  lilybuzzer_init(pins->buzzer); 
  nrf_gpio_cfg_input(pins->puzzle_select, NRF_GPIO_PIN_PULLUP);
  //sx1509_pin_config_input_pullup(sx1509_i2c_addr, pins->puzzle_select);

  sx1509_pin_config_output(sx1509_i2c_addr,  pins->led_b); 
  sx1509_pin_config_output(sx1509_i2c_addr,  pins->led_r); 
  sx1509_pin_config_output(sx1509_i2c_addr,  pins->led_g); 

  app_timer_create(&loop_timer, APP_TIMER_MODE_SINGLE_SHOT, morse_puzzle_continue);
  keypad_clear_input_record(); 
}

void morse_puzzle_start(void) {
  keypad_start_scanning(); 
  morse_solution_gen(); //generate a random solution
  is_puzzle_complete = false; 
  morse_set_LED_off(); 
  keypad_clear_input_record();
}


void morse_solution_gen(void) {
  for (uint8_t i = 0; i < SOLUTION_LENGTH; i++) {
    solution_str[i] = '0' + (rand() % 10); // digits 0-9
  }
  if (debug) printf("\n");
  solution_str[SOLUTION_LENGTH] = '\0'; 
    if (debug) printf("MORSE: Solution generated: %s\n", solution_str);
}

void morse_puzzle_continue(void* _unused) {
  keypad_start_scanning();
  // Reset if puzzle select is pressed
  if (!nrf_gpio_pin_read(pins->puzzle_select)) {
    if (debug) printf("Morse Puzzle: Puzzle reset.\n");
    stop_buzzer();
    play_morse_message(solution_str,SOLUTION_LENGTH); 
    keypad_clear_input_record(); 
    keypad_start_scanning(); 
    morse_set_LED_off(); 
    //morse_set_LED_white(); //TODO: fix this. it is setting the led RED. but if i use morse_set_LED_green() it sets it to green idk
  }
  
  // Check if keypad input matches 
  if (check_sequence()) {
    morse_set_LED_green(); 
    is_puzzle_complete = true; 
    if (debug) printf("Morse puzzle: Success! \n");
    stop_buzzer(); 
    keypad_stop_scanning(); 
  }
  else {
    app_timer_start(loop_timer, APP_TIMER_TICKS(100), NULL);
  }
}

void morse_puzzle_stop(void) {
  if (debug) printf("MORSE: Puzzle stopped.\n");
  stop_buzzer(); 
  keypad_clear_input_record();
  keypad_stop_scanning(); 
  app_timer_stop(loop_timer); 
  if (!is_puzzle_complete) morse_set_LED_off(); 
}

bool morse_puzzle_is_complete(void) {
  return is_puzzle_complete;
} 