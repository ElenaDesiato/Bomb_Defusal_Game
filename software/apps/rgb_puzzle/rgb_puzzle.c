#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h> 

#include "nrf_delay.h"
#include "nrf_twi_mngr.h"
#include "app_timer.h"

#include "microbit_v2.h" 
#include "sx1509.h"
#include "neopixel.h" 
#include "rgb_puzzle.h"

#define NUM_ROUNDS 3                // Number of successful sequences to complete the puzzle
#define SOL_LENGTH 6                // Length of each color sequence
#define NEOPIXEL_CENTER_LED 0       // LED index for center feedback
#define NEOPIXEL_OUTER_LED_START 1  // Starting LED index for outer ring
#define NUM_OUTER_LEDS 6            // Number of LEDs in the outer ring used for sequence display
#define SHOW_COLOR_DURATION_MS 700  // How long each color in the sequence is shown
#define FEEDBACK_DURATION_MS 1000   // How long success/failure feedback is shown
#define BUTTON_DEBOUNCE_DELAY_MS 50 // Debounce delay for buttons

APP_TIMER_DEF(SOL_SEQ_SHOW_TIMER);
APP_TIMER_DEF(FEEDBACK_SHOW_TIMER);

static bool debug = true;

static uint8_t sx1509_i2c_addr = 0;
static rgb_puzzle_pins_t* puzzle_pins = NULL;
static bool puzzle_initialized = false;
static bool puzzle_active = false;
static bool puzzle_complete = false;

static rgb_puzzle_color_t curr_sol_seq[SOL_LENGTH];
static int curr_step = 0;      // Current round number (0 to NUM_ROUNDS-1)
static int dislay_idx = 0;      // Index for showing current instruction step-by-step

static rgb_puzzle_color_t player_input_seq[SOL_LENGTH];
static int input_idx = 0;       // Current length of player's input sequence

// Button debouncing helper
static uint8_t button_pins[4]; // Stores actual SX1509 pin numbers for R,G,B,Y buttons
static bool last_button_pressed_states[4]; // true = was pressed, false = was released

static rgb_puzzle_fsm_state_t curr_state = PUZZLE_STATE_IDLE;

static void sol_seq_show_timer_handler(void* unused);
static void feedback_timer_handler(void* unused);
static void show_sol_seq(void);
static void process_button_press(rgb_puzzle_color_t color_pressed);

static void set_input_feedback(bool success);

void gen_sol_seq(void) {
    if (debug) printf("RGB: Generating new instruction sequence for round %d.\n", curr_step + 1);
    if (debug) printf("Generated Sequence: ");
    for (int i = 0; i < SOL_LENGTH; i++) {
        curr_sol_seq[i] = (rgb_puzzle_color_t)(rand() % 4); // 0 to 3 for R,G,B,Y
        if (debug) printf(" [%d]:\n", curr_sol_seq[i]);  // TODO: make it print the colro name
    }
}

static void set_input_feedback(bool success) {
    neopixel_set_color(NEO_JEWEL, 0 , success ? COLOR_GREEN : COLOR_RED);
    // TODO: set a app timer to clear it
}

// Timer Handlers
static void sol_seq_show_timer_handler(void* unused) {
    if (dislay_idx < SOL_LENGTH) {
        if (debug) printf("RGB: Showing instruction color %d/%d\n", dislay_idx + 1, SOL_LENGTH);
        neopixel_set_color(NEO_JEWEL, NEOPIXEL_OUTER_LED_START + dislay_idx, curr_sol_seq[dislay_idx]);
        dislay_idx++;
        // Restart timer for the next color
        app_timer_start(SOL_SEQ_SHOW_TIMER, APP_TIMER_TICKS(SHOW_COLOR_DURATION_MS), NULL);
    } else {
        // Sequence finished showing
        if (debug) printf("RGB: Instruction sequence shown. Waiting for input.\n");
        neopixel_clear_all(NEO_JEWEL);
        curr_state = PUZZLE_STATE_WAITING_FOR_INPUT;
        input_idx = 0; // Reset for player input
    }
}

static void feedback_timer_handler(void* unused) {
    neopixel_clear(NEO_JEWEL, NEOPIXEL_CENTER_LED); // Clear feedback color from center LED
    app_timer_stop(FEEDBACK_SHOW_TIMER);
}

// Puzzle Logic functions
static void show_sol_seq(void) {
    dislay_idx = 0;
    clear_outer_leds();
    neopixel_clear_all(NEO_JEWEL); // Ensure center LED is off before sequence starts

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
    
   // Configure SX1509 pins for buttons
    sx1509_pin_config_input_pullup(sx1509_i2c_addr, puzzle_pins->red_button);
    sx1509_pin_config_input_pullup(sx1509_i2c_addr, puzzle_pins->green_button);
    sx1509_pin_config_input_pullup(sx1509_i2c_addr, puzzle_pins->blue_button);
    sx1509_pin_config_input_pullup(sx1509_i2c_addr, puzzle_pins->yellow_button);
    sx1509_pin_config_input_pullup(sx1509_i2c_addr, puzzle_pins->puzzle_select);

    neopixel_clear_all(NEO_JEWEL);

    app_timer_create(&SOL_SEQ_SHOW_TIMER, APP_TIMER_MODE_SINGLE_SHOT, sol_seq_show_timer_handler);
    app_timer_create(&FEEDBACK_SHOW_TIMER, APP_TIMER_MODE_SINGLE_SHOT, feedback_timer_handler);

    srand(12345);

    // Initialize button debounce states (false = released)
    for(int i=0; i<4; i++) {
        last_button_pressed_states[i] = false;
    }
    puzzle_initialized = true;
    puzzle_active = false;
    puzzle_complete = false;
    curr_state = PUZZLE_STATE_IDLE;

    if (debug) printf("RGB: Puzzle Initialized.\n");
}

void rgb_puzzle_start() {
    if (puzzle_active) {return;}    // Prevent starting if already active
    curr_step = 0;
    dislay_idx = 0;
    input_idx = 0;
    puzzle_complete = false;
    puzzle_active = false;
    curr_state = PUZZLE_STATE_IDLE;
    if (debug) printf("RGB: Puzzle Started.\n");
}

void rgb_puzzle_stop() {
    if (!puzzle_active) return;

    if (debug) printf("RGB: Puzzle Stop.\n");

    app_timer_stop(SOL_SEQ_SHOW_TIMER);
    app_timer_stop(FEEDBACK_SHOW_TIMER);
    neopixel_clear_all(NEO_JEWEL);
    puzzle_active = false;
    curr_state = PUZZLE_STATE_IDLE;

    curr_step = 0; 
    dislay_idx = 0; 
    input_idx = 0; 

    if (!puzzle_complete) {
            //TODO
    }
}

bool rgb_puzzle_is_complete(void) {
    return puzzle_complete;
}

bool rgb_puzzle_is_active(void) {
    return puzzle_active;
}

void rgb_puzzle_handler(void* unused) { 

    if (steps_done >= NUM_INSTRUCTIONS_TO_PLAY) {
        if (!puzzle_complete) {
            if (debug) printf("RGB: Puzzle COMPLETED!\n");
                puzzle_complete = true;
        }
            rgb_puzzle_stop();
            return;
        }

    // state logic
    switch (curr_state) {
        case PUZZLE_STATE_IDLE:
            break;

        case PUZZLE_STATE_START_ROUND:
            if (debug) printf("RGB: State -> START_ROUND (Round %d/%d)\n", curr_step + 1, NUM_ROUNDS);
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
                    process_button_press((rgb_puzzle_color_t)i); // TODO: change?
                }
                last_button_pressed_states[i] = curr_press_state;
            }
            break;

        case PUZZLE_STATE_CHECKING_SEQUENCE: // a transient state
            if (debug) printf("RGB: State -> CHECKING_SEQUENCE\n");
            {
                bool seq_match = true;
                if (debug) printf("Comparing sequences:\n");
                for (int i = 0; i < SOL_LENGTH; i++) {
                    if (debug) printf("  Expected: %d, Player: %d\n", curr_sol_seq[i], player_input_seq[i]);
                    if (player_input_seq[i] != curr_sol_seq[i]) {
                        seq_match = false;
                        // TODO: set neojewel red for a short while
                        break;
                    }
                }

                neopixel_clear_all(NEO_JEWEL); 
                // TODO: set new_jewel green for a short while
                set_input_feedback(seq_match); // Show Green/Red on center LED
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
            curr_step++;
            if (curr_step < NUM_ROUNDS) {
                curr_state = PUZZLE_STATE_START_ROUND; // Proceed to next round
            } else {
                // All rounds complete successfully
                curr_state = PUZZLE_STATE_GAME_OVER_SUCCESS;
                puzzle_complete = true;
                puzzle_active = false; // Puzzle is over
                if (debug) printf("RGB: All rounds complete! Puzzle Success!\n");
                neopixel_set_color_all(NEO_JEWEL, COLOR_GREEN); // Show success on all LEDs
                curr_state = PUZZLE_STATE_IDLE; // Then idle
            }

        case PUZZLE_STATE_ROUND_FAILURE_FEEDBACK:
            // Player failed the round, retry the same instruction sequence
            if (debug) printf("RGB: Round failed. Retrying same instruction.\n");
            curr_state = PUZZLE_STATE_START_ROUND; // This will re-show the current (failed) instruction
            break;

        case PUZZLE_STATE_GAME_OVER_SUCCESS:
            curr_state = PUZZLE_STATE_IDLE;
            if (debug) printf("RGB: State -> GAME_OVER_SUCCESS (Puzzle Complete)\n");
            break;

        default:
            if (debug) printf("RGB: Error - Unknown puzzle state: %d. Going back to IDLE\n", curr_state);
            curr_state = PUZZLE_STATE_IDLE;
            break;
    }
}

static void process_button_press(rgb_puzzle_color_t color_pressed) {
    if (curr_state != PUZZLE_STATE_WAITING_FOR_INPUT) { return;}
    if (input_idx >= SOL_LENGTH) { return;}

    // TODO: feedback for input (set center led to the color pressed)

    player_input_seq[input_idx] = color_pressed;
    input_idx++;
    if (debug) printf("RGB: Player input %d/%d: Color %d registered.\n", input_idx, SOL_LENGTH, color_pressed);

    if (input_idx == SOL_LENGTH) {
        // Player has entered the full sequence length
        if (debug) printf("RGB: Full sequence entered by player. Changing state to CHECKING_SEQUENCE.\n");
        curr_state = PUZZLE_STATE_CHECKING_SEQUENCE;
    }
}