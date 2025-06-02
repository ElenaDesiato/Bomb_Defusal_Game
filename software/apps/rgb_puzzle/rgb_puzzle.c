#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h> 

#include "nrf_delay.h"
#include "nrf_twi_mngr.h"
#include "app_timer.h"
#include "app_error.h" // For APP_ERROR_CHECK
#include "nrf_log.h"   // For NRF_LOG_INFO, if used

#include "microbit_v2.h" 
#include "sx1509.h"
#include "neopixel_ring.h" // might change the driver later...
#include "rgb_puzzle.h"

// --- Configuration ---
#define NUM_ROUNDS 3                // Number of successful sequences to complete the puzzle
#define SOL_LENGTH 4        // Length of each color sequence
#define NEOPIXEL_CENTER_LED 0       // LED index for center feedback
#define NEOPIXEL_OUTER_LED_START 1  // Starting LED index for outer ring
#define NUM_OUTER_LEDS 6            // Number of LEDs in the outer ring used for sequence display
#define SHOW_COLOR_DURATION_MS 700  // How long each color in the sequence is shown
#define FEEDBACK_DURATION_MS 1000   // How long success/failure feedback is shown
#define BUTTON_DEBOUNCE_DELAY_MS 50 // Debounce delay for buttons

APP_TIMER_DEF(SOL_SEQ_SHOW_TIMER);
APP_TIMER_DEF(FEEDBACK_SHOW_TIMER);

NRF_TWI_MNGR_DEF(twi_mngr_instance, 5, 0);

static bool debug = true;

static uint8_t sx1509_i2c_addr = 0;
static rgb_puzzle_pins_t* puzzle_pins = NULL;
static bool puzzle_initialized = false;
static bool puzzle_active = false;
static bool puzzle_complete = false;

static rgb_puzzle_color_t curr_sol_seq[SOL_LENGTH];
static int curr_round = 0;      // Current round number (0 to NUM_ROUNDS-1)
static int dislay_idx = 0;      // Index for showing current instruction step-by-step

static rgb_puzzle_color_t player_input_seq[SOL_LENGTH];
static int input_idx = 0;       // Current length of player's input sequence

// Button debouncing helper
static uint8_t button_pins[4]; // Stores actual SX1509 pin numbers for R,G,B,Y buttons
static bool last_button_pressed_states[4]; // true = was pressed, false = was released

static rgb_puzzle_fsm_state_t curr_state = PUZZLE_STATE_IDLE;

// --- Forward declarations ---
static void sol_seq_show_timer_handler(void* p_context);
static void feedback_timer_handler(void* p_context);
static void show_sol_seq(void);
static void process_button_press(rgb_puzzle_color_t color_pressed);

static color_name_t color_mapping(rgb_puzzle_color_t puzzle_color); // to have the R, G, B, Y's RGB values in the color map in neopixel driver

static void set_outer_led_color(int jewel_led_idx, rgb_puzzle_color_t p_color);
static void clear_outer_leds(void);

static void set_center_led_feedback(bool success);

static void gen_sol_seq(void);

void gen_sol_seq(void) {
    if (debug) printf("RGB: Generating new instruction sequence for round %d.\n", curr_round + 1);
    for (int i = 0; i < SOL_LENGTH; i++) {
        curr_sol_seq[i] = (rgb_puzzle_color_t)(rand() % 4); // 0 to 3 for R,G,B,Y
        if (debug) printf("  Inst [%d]: %d\n", i, curr_sol_seq[i]);
    }
}

// Neopixel Helper functions
static color_name_t color_mapping(rgb_puzzle_color_t puzzle_color) {
    switch (puzzle_color) {
        case R: return COLOR_RED;
        case G: return COLOR_GREEN;
        case B: return COLOR_BLUE;
        case Y: return COLOR_YELLOW;
        default: return COLOR_BLACK; // Should not happen
    }
}

static void set_outer_led_color(int jewel_led_idx, rgb_puzzle_color_t p_color) {
    // jewel_led_idx is 0 to NUM_OUTER_LEDS-1
    if (jewel_led_idx >= NUM_OUTER_LEDS) return;
    neopixel_set_color(NEOPIXEL_OUTER_LED_START + jewel_led_idx, color_mapping(p_color));
}

static void clear_outer_leds(void) {
    for (int i = 0; i < NUM_OUTER_LEDS; i++) {
        neopixel_clear(NEOPIXEL_OUTER_LED_START + i);
    }
}

static void set_center_led_feedback(bool success) {
    neopixel_set_color(NEOPIXEL_CENTER_LED, success ? COLOR_GREEN : COLOR_RED);
}

// Timer Handlers
static void sol_seq_show_timer_handler(void* p_context) {
    UNUSED_PARAMETER(p_context);
    clear_outer_leds(); // Clear previous LED shown in sequence

    if (dislay_idx < SOL_LENGTH) {
        if (debug) printf("RGB: Showing instruction color %d/%d\n", dislay_idx + 1, SOL_LENGTH);
        set_outer_led_color(dislay_idx % NUM_OUTER_LEDS, curr_sol_seq[dislay_idx]);
        dislay_idx++;
        // Restart timer for the next color
        app_timer_start(SOL_SEQ_SHOW_TIMER, APP_TIMER_TICKS(SHOW_COLOR_DURATION_MS), NULL);
    } else {
        // Sequence finished showing
        if (debug) printf("RGB: Instruction sequence shown. Waiting for input.\n");
        curr_state = PUZZLE_STATE_WAITING_FOR_INPUT;
        input_idx = 0; // Reset for player input
    }
}

static void feedback_timer_handler(void* p_context) {
    UNUSED_PARAMETER(p_context);
    neopixel_clear(NEOPIXEL_CENTER_LED); // Clear feedback color from center LED

    if (curr_state == PUZZLE_STATE_ROUND_SUCCESS_FEEDBACK) {
        curr_round++;
        if (curr_round < NUM_ROUNDS) {
            curr_state = PUZZLE_STATE_START_ROUND; // Proceed to next round
        } else {
            // All rounds complete successfully
            curr_state = PUZZLE_STATE_GAME_OVER_SUCCESS;
            puzzle_complete = true;
            puzzle_active = false; // Puzzle is over
            if (debug) printf("RGB: All rounds complete! Puzzle Success!\n");
            // Optional: Final success animation (e.g., flash all green)
            for(int i=0; i<3; ++i) { // Flash green 3 times
                neopixel_set_color_all(COLOR_GREEN); nrf_delay_ms(150);
                neopixel_clear_all(); nrf_delay_ms(150);
            }
            curr_state = PUZZLE_STATE_IDLE; // Then idle
        }
    } else if (curr_state == PUZZLE_STATE_ROUND_FAILURE_FEEDBACK) {
        // Player failed the round, retry the same instruction sequence
        if (debug) printf("RGB: Round failed. Retrying same instruction.\n");
        curr_state = PUZZLE_STATE_START_ROUND; // This will re-show the current (failed) instruction
    } else if (curr_state == PUZZLE_STATE_GAME_OVER_SUCCESS) {
        // This case handles if feedback timer was used for the final success animation
        curr_state = PUZZLE_STATE_IDLE;
    }
}

// Puzzle Logic functions
static void show_sol_seq(void) {
    dislay_idx = 0;
    clear_outer_leds();
    neopixel_clear(NEOPIXEL_CENTER_LED); // Ensure center LED is off before sequence starts
    curr_state = PUZZLE_STATE_SHOWING_INSTRUCTION;
    if (debug) printf("RGB: Starting to show instruction sequence.\n");
    // Start the timer to show the first color (slight delay before first color appears)
    app_timer_start(SOL_SEQ_SHOW_TIMER, APP_TIMER_TICKS(SHOW_COLOR_DURATION_MS / 2), NULL);
}


void rgb_puzzle_init(uint8_t sx1509_addr, rgb_puzzle_pins_t* p_pins, bool enable_debug) {
    if (puzzle_initialized) return;

    debug = enable_debug;
    puzzle_pins = p_pins;
    sx1509_i2c_addr = sx1509_addr;

    // Store button HW pins for easier access in polling loop
    button_pins[R] = puzzle_pins->red_button;
    button_pins[G] = puzzle_pins->green_button;
    button_pins[B] = puzzle_pins->blue_button;
    button_pins[Y] = puzzle_pins->yellow_button;

    // Initialize I2C manager (DELETE THIS DURING INTEGRATION)
    if (!nrf_twi_mngr_is_initialized(&twi_mngr_instance)) {
        nrf_drv_twi_config_t i2c_config = NRF_DRV_TWI_DEFAULT_CONFIG;
        i2c_config.scl = EDGE_P19; 
        i2c_config.sda = EDGE_P20;
        i2c_config.frequency = NRF_DRV_TWI_FREQ_100K;
        i2c_config.interrupt_priority = APP_IRQ_PRIORITY_LOW; 
        APP_ERROR_CHECK(nrf_twi_mngr_init(&twi_mngr_instance, &i2c_config));
    }
    
    // SX1509 GPIO expander init
    sx1509_init(sx1509_i2c_addr, &twi_mngr_instance);
    if (debug) printf("SX1509 is %s\n", sx1509_is_connected(sx1509_i2c_addr, &twi_mngr_instance) ? "connected" : "not connected");

    // Configure SX1509 pins for buttons
    sx1509_pin_config_input_pullup(sx1509_i2c_addr, puzzle_pins->red_button);
    sx1509_pin_config_input_pullup(sx1509_i2c_addr, puzzle_pins->green_button);
    sx1509_pin_config_input_pullup(sx1509_i2c_addr, puzzle_pins->blue_button);
    sx1509_pin_config_input_pullup(sx1509_i2c_addr, puzzle_pins->yellow_button);
    sx1509_pin_config_input_pullup(sx1509_i2c_addr, puzzle_pins->puzzle_select);

    // Neopixel Jewel Init
    neopixel_ring_init(puzzle_pins->neopixel_jewel); // CHANGE THIS AFTER WE UPDATE THE NEOPIXEL DRIVER
    neopixel_clear_all();

    // App timer module should be initialized once, typically in main.c.
    // app_timer_init(); // If not called elsewhere, uncomment. Assuming it's in main.
    uint32_t err_code;
    err_code = app_timer_create(&SOL_SEQ_SHOW_TIMER, APP_TIMER_MODE_SINGLE_SHOT, sol_seq_show_timer_handler);
    APP_ERROR_CHECK(err_code);
    err_code = app_timer_create(&FEEDBACK_SHOW_TIMER, APP_TIMER_MODE_SINGLE_SHOT, feedback_timer_handler);
    APP_ERROR_CHECK(err_code);

    srand(12345);

    // Initialize button debounce states (false = released)
    for(int i=0; i<4; i++) {
        last_button_pressed_states[i] = false;
    }
    puzzle_initialized = true;
    puzzle_active = false;
    puzzle_complete = false;
    curr_state = PUZZLE_STATE_IDLE;

    if (debug) printf("RGB: Puzzle module initialized.\n");
}

void rgb_puzzle_start() {
    if (!puzzle_initialized) {
        if (debug) printf("RGB: Attempt to start puzzle but not initialized.\n");
        return;
    }
    if (puzzle_active && !puzzle_complete) {
        if (debug) printf("RGB: Attempt to start puzzle already active.\n");
        return;
    }

    if (debug) printf("RGB: Starting puzzle.\n");
    puzzle_active = true;
    puzzle_complete = false;
    curr_round = 0;
    input_idx = 0;
    curr_state = PUZZLE_STATE_START_ROUND;
    // the rest job goes to FSM
}

void rgb_puzzle_end() {
    if (!puzzle_initialized) return;
    if (debug) printf("RGB: Ending puzzle.\n");

    app_timer_stop(SOL_SEQ_SHOW_TIMER);
    app_timer_stop(FEEDBACK_SHOW_TIMER);

    neopixel_clear_all();  // CHANGE THIS AFTER UPDATING NEOPIXEL DRIVER

    puzzle_active = false;
    curr_state = PUZZLE_STATE_IDLE;
}

bool rgb_puzzle_is_complete(void) {
    return puzzle_complete;
}

bool rgb_puzzle_is_active(void) {
    return puzzle_active;
}

// the FSM machine handler
void rgb_puzzle_process_state(void) { 
    if (!puzzle_initialized) return;

    if (puzzle_active) {
      if (sx1509_pin_read(sx1509_i2c_addr, puzzle_pins->puzzle_select) == false) { // Button pressed
          nrf_delay_ms(BUTTON_DEBOUNCE_DELAY_MS); // debounce
          if (sx1509_pin_read(sx1509_i2c_addr, puzzle_pins->puzzle_select) == false) {
               if (debug) printf("RGB: Puzzle select button pressed. Ending puzzle.\n");
               rgb_puzzle_end();
               return;
          }
      }
    } else { return; }

    // FSM logic
    switch (curr_state) {
        case PUZZLE_STATE_IDLE:
            break;

        case PUZZLE_STATE_START_ROUND:
            if (debug) printf("RGB: State -> START_ROUND (Round %d/%d)\n", curr_round + 1, NUM_ROUNDS);
            gen_sol_seq();
            input_idx = 0; // Reset player input for the new round
            show_sol_seq(); // This changes state to PUZZLE_STATE_SHOWING_INSTRUCTION
            break;

        case PUZZLE_STATE_SHOWING_INSTRUCTION:
            break;

        case PUZZLE_STATE_WAITING_FOR_INPUT:
            // Poll buttons for input
            for (int i = 0; i < 4; i++) { // Iterate R, G, B, Y buttons
                bool curr_press_state = (sx1509_pin_read(sx1509_i2c_addr, button_pins[i]) == false); // true if pressed (low)

                if (curr_press_state && !last_button_pressed_states[i]) { // Rising edge of press (was released, now pressed)
                    if (debug) printf("RGB: Button %d pressed by player.\n", i);
                    process_button_press((rgb_puzzle_color_t)i);
                }
                last_button_pressed_states[i] = curr_press_state;
            }
            break;

        case PUZZLE_STATE_CHECKING_SEQUENCE: // This is a transient state
            if (debug) printf("RGB: State -> CHECKING_SEQUENCE\n");
            {
                bool seq_match = true;
                if (debug) printf("Comparing sequences:\n");
                for (int i = 0; i < SOL_LENGTH; i++) {
                    if (debug) printf("  Expected: %d, Player: %d\n", curr_sol_seq[i], player_input_seq[i]);
                    if (player_input_seq[i] != curr_sol_seq[i]) {
                        seq_match = false;
                        break;
                    }
                }

                clear_outer_leds(); // Clear LEDs that showed player input
                set_center_led_feedback(seq_match); // Show Green/Red on center LED
                app_timer_start(FEEDBACK_SHOW_TIMER, APP_TIMER_TICKS(FEEDBACK_DURATION_MS), NULL);

                if (seq_match) {
                    if (debug) printf("RGB: Sequence CORRECT!\n");
                    curr_state = PUZZLE_STATE_ROUND_SUCCESS_FEEDBACK;
                } else {
                    if (debug) printf("RGB: Sequence INCORRECT.\n");
                    curr_state = PUZZLE_STATE_ROUND_FAILURE_FEEDBACK;
                }
            }
            break;

        case PUZZLE_STATE_ROUND_SUCCESS_FEEDBACK:
        case PUZZLE_STATE_ROUND_FAILURE_FEEDBACK:
            // These states are driven by FEEDBACK_SHOW_TIMER.
            // Timer handler will transition to next appropriate state (START_ROUND or GAME_OVER_SUCCESS).
            break;

        case PUZZLE_STATE_GAME_OVER_SUCCESS:
            if (debug) printf("RGB: State -> GAME_OVER_SUCCESS (Puzzle Complete)\n");
            break;

        default:
            if (debug) printf("RGB: Error - Unknown puzzle state: %d. Going back to IDLE\n", curr_state);
            curr_state = PUZZLE_STATE_IDLE;
            break;
    }
}

static void process_button_press(rgb_puzzle_color_t color_pressed) {
    // This function is called upon a confirmed button press event.
    if (curr_state != PUZZLE_STATE_WAITING_FOR_INPUT || input_idx >= SOL_LENGTH) {
        // end if:
        //     not the right state
        //     input reaches the end
        return;
    }

    // feedback for input
    set_outer_led_color(input_idx % NUM_OUTER_LEDS, color_pressed);

    player_input_seq[input_idx] = color_pressed;
    input_idx++;
    if (debug) printf("RGB: Player input %d/%d: Color %d registered.\n", input_idx, SOL_LENGTH, color_pressed);

    if (input_idx == SOL_LENGTH) {
        // Player has entered the full sequence length
        if (debug) printf("RGB: Full sequence entered by player. Changing state to CHECKING_SEQUENCE.\n");
        curr_state = PUZZLE_STATE_CHECKING_SEQUENCE;
    }
}