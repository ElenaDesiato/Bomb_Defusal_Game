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

NRF_TWI_MNGR_DEF(twi_mngr_instance, 1, 0);

static uint8_t sx1509_i2c_addr = SX1509_ADDR_10; 
static morse_puzzle_pins_t* pins = NULL; 
static uint8_t solution_length = 0; 
static char* solution_str = "";
static bool puzzle_complete = false; 

static bool debug = true; 

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

// Helper function to check if keypad input matches solution
static bool check_sequence(char* sol, uint8_t sol_len){
  char* input = keypad_get_input();
  uint8_t input_len = keypad_get_input_length();
  if (input_len < sol_len) {
    return false;
  }

  for (uint8_t i = 0; i < sol_len; i++) {
    // Some input is wrong -> reset & LED red
    if (input[i] != sol[i]){
      set_LED_red(); 
      keypad_clear_input_record();
      printf("Failure\n");
      return false;
    }
  }
  // Correct input -> LED green
  set_LED_green(); 
  puzzle_complete = true; 
  printf("Success!! \n");
  stop_buzzer();
  return true; 
}

void morse_puzzle_init(uint8_t i2c_addr, morse_puzzle_pins_t* puzzle_pins) {
  sx1509_i2c_addr = i2c_addr; 
  // i2c config & initialization
  nrf_drv_twi_config_t i2c_config = NRF_DRV_TWI_DEFAULT_CONFIG;
  i2c_config.scl = EDGE_P19;
  i2c_config.sda = EDGE_P20;
  i2c_config.frequency = NRF_DRV_TWI_FREQ_100K;
  i2c_config.interrupt_priority = 0;
  nrf_twi_mngr_init(&twi_mngr_instance, &i2c_config);

  sx1509_init(sx1509_i2c_addr,  &twi_mngr_instance);
  if (debug) printf("SX1509 is %s\n", sx1509_is_connected(sx1509_i2c_addr,  &twi_mngr_instance) ? "connected" : "not connected");

  // Pins 
  pins = puzzle_pins; 
  solution_length = 0; 
  solution_str = "";
  puzzle_complete = false; 

  app_timer_init(); 
  keypad_init(sx1509_i2c_addr,pins->rows, pins->cols); 
  lilybuzzer_init(pins->buzzer); 
  sx1509_pin_config_input_pullup(sx1509_i2c_addr, pins->puzzle_select);

  sx1509_pin_config_output(sx1509_i2c_addr,  pins->led_b); 
  sx1509_pin_config_output(sx1509_i2c_addr,  pins->led_r); 
  sx1509_pin_config_output(sx1509_i2c_addr,  pins->led_g); 
}

// TO DO: better way of generating a solution
bool morse_puzzle_start(void) {
  solution_length = 4; 
  solution_str = "1234"; 

  while(1) {
    // Reset if puzzle select is pressed
    if (!sx1509_pin_read(sx1509_i2c_addr,  pins->puzzle_select)) {
      if (debug) {printf("puzzle select button pressed. pin value: %d \n",sx1509_pin_read(sx1509_i2c_addr,  pins->puzzle_select) );}
      printf("Puzzle reset.\n");
      stop_buzzer();
      play_morse_message(solution_str,solution_length); 
      keypad_clear_input_record(); 
      set_LED_white();
    }
    // Check if keypad input matches 
    if (check_sequence(solution_str, solution_length)) return true; 
    nrf_delay_ms(100);
  }
  return false; 
}

bool morse_puzzle_is_complete(void) {
  return puzzle_complete;
} 