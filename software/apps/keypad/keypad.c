#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>

#include "app_timer.h"
#include "nrf_delay.h"
#include "nrf_twi_mngr.h"

#include "microbit_v2.h"
#include "../sx1509/sx1509.h"

#define MAX_RECORD_LENGTH 10

APP_TIMER_DEF(keypad_scanning_timer);

static const uint8_t* row_pins = NULL; 
static const uint8_t* col_pins = NULL;
static uint8_t sx1509_i2c_addr = 0; 

static char key_record[MAX_RECORD_LENGTH]; // Array to record the key sequence
static uint8_t record_index = 0;           // Index to keep track of the current position in the record
static char key_prev = '\0'; 

static const char key_map[4][3] = {
  {'1', '2', '3'},  
  {'4', '5', '6'}, 
  {'7', '8', '9'}, 
  {'*', '0', '#'} 
};

static void keypad_read_input(void* _unused);

static bool debug = true; 

static bool is_scanning = false;

void keypad_start_scanning(void) {
  if (is_scanning) {return;}

  record_index = 0;
  is_scanning = true;
  app_timer_start(keypad_scanning_timer, APP_TIMER_TICKS(100), NULL); 
}

bool keypad_init(uint8_t i2c_addr, const uint8_t* rows, const uint8_t* cols) {
  row_pins = rows; 
  col_pins = cols;
  sx1509_i2c_addr = i2c_addr; 
  is_scanning = false;
  for (int i = 0; i < 4; i++) {
    sx1509_pin_config_output(sx1509_i2c_addr, row_pins[i]);
    sx1509_pin_write(sx1509_i2c_addr, row_pins[i], true); // set high at init
  }

  // Col pins -> Input (pull up)
  for (int i = 0; i < 3; i++) {
    sx1509_pin_config_input_pullup(sx1509_i2c_addr, col_pins[i]);
  }

  //app_timer_init();
  app_timer_create(&keypad_scanning_timer, APP_TIMER_MODE_REPEATED, keypad_read_input);
  if (debug) printf("KEYPAD: INITIALIZED\n");
  return true;
}

static void keypad_read_input(void* _unused) {
    char key_current = '\0';
    //if (debug) printf("KEYPAD: scanning...\n");
    // Keyboard Scanning
    for (int r = 0; r < 4; r++) {
      sx1509_pin_write(sx1509_i2c_addr, row_pins[r], false);  // Scanned Row: Set Output as low
      nrf_delay_us(10);

      for (int c = 0; c < 3; c++) {
        if (!sx1509_pin_read(sx1509_i2c_addr, col_pins[c])) { // Scan Cols: low -> col is pressed
          key_current = key_map[r][c];
          break; 
        }
      }
      sx1509_pin_write(sx1509_i2c_addr, row_pins[r], true); // Write current row back to high

      if (key_current != '\0') { // If key already found, end row scanning early
          break;
      }
    }

    if (key_current != '\0' && key_current != key_prev) { 
        key_prev = key_current;
          if (record_index < MAX_RECORD_LENGTH) {
            key_record[record_index++] = key_current;
            key_record[record_index] = '\0'; // Keep it null-terminated
        }
        if (debug) printf("Key_current: %c\n",key_current);
    } else if (key_current == '\0') {
      key_prev = '\0';
    } 
}

void keypad_clear_input_record(void) {
  for (uint8_t i = 0; i < MAX_RECORD_LENGTH; i++) {
    key_record[i] = '\0';
  }
  record_index = 0;  // reset the index
}

char* keypad_get_input(void) {
  return key_record;
}

uint8_t keypad_get_input_length(void) {
  return record_index;
}

void print_keypad_input(void) {
  printf("Recorded keypad input sequence: ");
  for (uint8_t i = 0; i < record_index; i++) {
    if (key_record[i] != '\0') {
    printf("%c ", key_record[i]);}
  }
  printf("\n");
}

void keypad_stop_scanning(void) {
  if (debug) printf("Stop scanning called \n");
  is_scanning = false;
  app_timer_stop(keypad_scanning_timer); // Stop the scanning timer
}