// main.c (Example Adaptation)
#include "microbit_v2.h"
#include "nrf_delay.h"
#include "app_timer.h"    // For app_timer_init
#include "app_error.h"  // For APP_ERROR_CHECK
#include "sx1509.h"

#include "rgb_puzzle.h"

int main(void) {

    printf("broad start...\n");
  
    ret_code_t err_code;

    // Initialize app_timer module (once)
    err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);
    printf("RGB Puzzle Game Starting...\n");

  // Define puzzle pins
  rgb_puzzle_pins_t pins = {
    .red_button = 0,
    .yellow_button = 1,
    .green_button = 2,
    .blue_button = 3,
    .puzzle_select = 4, 
    .neopixel_jewel = EDGE_P11
  };
  bool debug_mode = true;

  // Initialize the RGB Puzzle module
  // SX1509_ADDR_00 should be defined in sx1509.h or elsewhere
  rgb_puzzle_init(SX1509_ADDR_00, &pins, debug_mode);

  // Start the puzzle sequence
  rgb_puzzle_start();

  while (1) {
    rgb_puzzle_process_state(); // the FSM machine for RGB puzzle

    if (rgb_puzzle_is_complete()) {
        printf("Main: RGB puzzle fully complete!\n");

        nrf_delay_ms(3000);
        printf("Main: Restarting puzzle...\n");
        rgb_puzzle_start(); // Start a new game

    } else if (!rgb_puzzle_is_active() && current_puzzle_state == PUZZLE_STATE_IDLE) {
        // This condition means the puzzle was ended (e.g. by select button) or completed and went to IDLE
        // And rgb_puzzle_start() hasn't been called again yet for a new game.
    }

    nrf_delay_ms(20); // Adjust delay as needed for responsiveness vs power saving
  }
}