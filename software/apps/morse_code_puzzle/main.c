#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "nrf_delay.h"
#include "nrf_twi_mngr.h"
#include "app_timer.h"

#include "microbit_v2.h"
#include "../lilybuzzer/lilybuzzer.h"
#include "../sx1509/sx1509.h"
#include "../keypad/keypad.h"

NRF_TWI_MNGR_DEF(twi_mngr_instance, 1, 0);

// pins on the gpio expander
#define PIN_PUZZLE_SELECT 15 
#define PIN_LED_G 12 
#define PIN_LED_B 13
#define PIN_LED_R 14
// pins 0-6 are for the keypad
// pin 10 is for the lilybuzzer

// Helper functions to turn led green, red, white respectively
static void set_LED_red() {
  sx1509_pin_clear(PIN_LED_R); 
  sx1509_pin_set(PIN_LED_G); 
  sx1509_pin_set(PIN_LED_B); 
}
static void set_LED_green() {
  sx1509_pin_set(PIN_LED_R); 
  sx1509_pin_clear(PIN_LED_G); 
  sx1509_pin_set(PIN_LED_B); 
}
static void set_LED_white() {
  sx1509_pin_clear(PIN_LED_R); 
  sx1509_pin_clear(PIN_LED_G); 
  sx1509_pin_clear(PIN_LED_B); 
}

// Helper function to check if keypad input matches solution
static bool check_sequence(char* sol, uint32_t sol_len){
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
  printf("Success!! \n");
  stop_buzzer();
  return true; 
}

int main(void) {
  printf("Board started!\n");

  // i2c config & initialization
  nrf_drv_twi_config_t i2c_config = NRF_DRV_TWI_DEFAULT_CONFIG;
  i2c_config.scl = EDGE_P19;
  i2c_config.sda = EDGE_P20;
  i2c_config.frequency = NRF_DRV_TWI_FREQ_100K;
  i2c_config.interrupt_priority = 0;
  nrf_twi_mngr_init(&twi_mngr_instance, &i2c_config);

  sx1509_init(&twi_mngr_instance);
  printf("SX1509 is %s\n", sx1509_is_connected(&twi_mngr_instance) ? "connected" : "not connected");

  app_timer_init(); 
  keypad_init(); 
  lilybuzzer_init(); 

  sx1509_pin_config_output(PIN_LED_B); 
  sx1509_pin_config_output(PIN_LED_R); 
  sx1509_pin_config_output(PIN_LED_G); 

  // TO DO: puzzle selection
  int solution_length = 4; 
  char* solution_str = "1234"; 


  while(1) {
    // Reset if puzzle select is pressed
    if (sx1509_pin_read(PIN_PUZZLE_SELECT)) {
      printf("Puzzle reset.\n");
      stop_buzzer();
      play_morse_message(solution_str,solution_length); 
      keypad_clear_input_record(); 
      set_LED_white();
    }
    // Check if keypad input matches 
    check_sequence(solution_str, solution_length); 
    nrf_delay_ms(100);
  }
}