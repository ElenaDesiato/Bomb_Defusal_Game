#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

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
static morse_puzzle_pins_t* pins = NULL; 

static uint8_t solution_length = 0; 
static char* solution_str = "";
static bool is_puzzle_complete = false; 

static bool debug = false; 


// Helper functions to turn led green, red, white respectively
static void set_LED_red() {
  sx1509_pin_clear(sx1509_i2c_addr,  pins->led_r); 
  sx1509_pin_set(sx1509_i2c_addr,  pins->led_g); 
  sx1509_pin_set(sx1509_i2c_addr,  pins->led_b); 
}
static void set_LED_green() {
  sx1509_pin_set(sx1509_i2c_addr,  pins->led_r); 
  sx1509_pin_clear(sx1509_i2c_addr,  pins->led_g); 
  sx1509_pin_set(sx1509_i2c_addr,  pins->led_b); 
}
static void set_LED_white() {
  sx1509_pin_clear(sx1509_i2c_addr,  pins->led_r); 
  sx1509_pin_clear(sx1509_i2c_addr,  pins->led_g); 
  sx1509_pin_clear(sx1509_i2c_addr,  pins->led_b); 
}  
static void set_LED_off() {
  sx1509_pin_set(sx1509_i2c_addr,  pins->led_r); 
  sx1509_pin_set(sx1509_i2c_addr,  pins->led_g); 
  sx1509_pin_set(sx1509_i2c_addr,  pins->led_b); 
}

// Helper function to check if keypad input matches solution
static bool check_sequence(char* sol, uint8_t sol_len){
  char* input = keypad_get_input();
  uint8_t input_len = keypad_get_input_length();
  if (input_len < sol_len) {
    return false;
  }

  for (uint8_t i = 0; i < sol_len; i++) {
    if (input[i] != sol[i]){
      set_LED_red(); 
      keypad_clear_input_record();
      if (debug) printf("Morse puzzle: Failure\n");
      return false;
    }
  }
  return true; 
}

void morse_puzzle_init(uint8_t i2c_addr, nrf_twi_mngr_t* twi_mgr_instance, morse_puzzle_pins_t* puzzle_pins, bool p_debug) {
  debug = p_debug;
  sx1509_i2c_addr = i2c_addr; 

  sx1509_init(sx1509_i2c_addr, twi_mgr_instance);
  if (debug) printf("SX1509 is %s\n", sx1509_is_connected(sx1509_i2c_addr,  twi_mgr_instance) ? "connected" : "not connected");

  pins = puzzle_pins; 
  solution_length = 0; 
  solution_str = "";
  is_puzzle_complete = false; 

  //app_timer_init(); 
  keypad_init(sx1509_i2c_addr,pins->rows, pins->cols); 
  lilybuzzer_init(pins->buzzer); 
  sx1509_pin_config_input_pullup(sx1509_i2c_addr, pins->puzzle_select);

  sx1509_pin_config_output(sx1509_i2c_addr,  pins->led_b); 
  sx1509_pin_config_output(sx1509_i2c_addr,  pins->led_r); 
  sx1509_pin_config_output(sx1509_i2c_addr,  pins->led_g); 

  //app_timer_create(&led_turn_off, APP_TIMER_MODE_SINGLE_SHOT,turn_off_LED);
  app_timer_create(&loop_timer, APP_TIMER_MODE_SINGLE_SHOT, morse_puzzle_continue);
}

// TO DO: better way of generating a solution
void morse_puzzle_start(void) {
  solution_length = 4; 
  solution_str = "1234"; 
  is_puzzle_complete = false; 
  set_LED_off(); 
  keypad_clear_input_record();
}

void morse_puzzle_continue(void) {
  // Reset if puzzle select is pressed
  if (!sx1509_pin_read(sx1509_i2c_addr,  pins->puzzle_select)) {
    if (debug) printf("Morse Puzzle: Puzzle reset.\n");
    stop_buzzer();
    play_morse_message(solution_str,solution_length); 
    keypad_clear_input_record(); 
    keypad_start_scanning(); 
    set_LED_white();
  }
  // Check if keypad input matches 
  if (check_sequence(solution_str, solution_length)) {
    set_LED_green(); 
    is_puzzle_complete = true; 
    if (debug) printf("Morse puzzle: Success! \n");
    stop_buzzer(); 
    keypad_stop_scanning(); 
  }
  else {
    app_timer_start(loop_timer, APP_TIMER_TICKS(10), NULL);
  }
}

void morse_puzzle_stop(void) {
  stop_buzzer(); 
  keypad_clear_input_record();
  keypad_stop_scanning(); 
  app_timer_stop(loop_timer); 
  if (!is_puzzle_complete) set_LED_off(); 
}

bool morse_puzzle_is_complete(void) {
  return is_puzzle_complete;
} 