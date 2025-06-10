#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h> 

#include "nrf_delay.h"
#include "nrf_twi_mngr.h"
#include "app_timer.h"
#include "app_error.h"

#include "microbit_v2.h" 
#include "sx1509.h"
#include "neopixel.h" 
#include "rgb_puzzle.h"
#include "rng.h"

#define NUM_ROUNDS 3                
#define SOL_LENGTH 6              
#define NEOPIXEL_CENTER_LED 0       // LED index for center feedback
#define NEOPIXEL_OUTER_LED_START 1  // Starting LED index for outer ring
#define NUM_OUTER_LEDS 6           
#define SHOW_COLOR_DURATION_MS 700  
#define FEEDBACK_DURATION_MS 1000  
#define BUTTON_DEBOUNCE_DELAY_MS 50  // Debounce delay for button

APP_TIMER_DEF(RGB_PUZZLE_HANDLER_TIMER); // Main timer as puzzle manager (this keeps calling the puzzle handler)
APP_TIMER_DEF(SOL_SEQ_SHOW_TIMER); // timer for showing the instruction sequence
APP_TIMER_DEF(FEEDBACK_SHOW_TIMER); // timer for showing feedback (success or fail)

static bool debug = true;

static uint8_t sx1509_i2c_addr = 0;
static const rgb_puzzle_pins_t* puzzle_pins = NULL;

static bool puzzle_initialized = false;
static bool puzzle_active = false;
static bool puzzle_complete = false;

static rgb_puzzle_color_t curr_sol_seq[SOL_LENGTH];
static int curr_step = 0;      // Current round number 
static int display_idx = 0;      // Index for showing current instruction sequence

static rgb_puzzle_color_t player_input_seq[SOL_LENGTH];
static int input_idx = 0;       // current input idx (starts from 0)

static uint8_t button_pins[4];  // Stores pin numbers for R,G,B,Y buttons
static bool last_button_pressed_states[4]; // true = was pressed, false = was released. used for debouncing

static rgb_puzzle_fsm_state_t curr_state = IDLE;

void rgb_puzzle_handler(void* unused);

// helper function to return color name as string for dubug printing
static const char* get_color_name_str(rgb_puzzle_color_t color) {
    switch(color) {
        case PUZZLE_COLOR_RED: return "RED";
        case PUZZLE_COLOR_GREEN: return "GREEN";
        case PUZZLE_COLOR_BLUE: return "BLUE";
        case PUZZLE_COLOR_YELLOW: return "YELLOW";
        default: return "ERROR";
    }
}

// clear outer LEDs of Neopixel jewel
static void clear_outer_leds() {
    for (int i = 0; i < NUM_OUTER_LEDS; i++) {
        neopixel_clear(NEO_JEWEL, NEOPIXEL_OUTER_LED_START + i);
    }
}

// generate random instruction sequence
void gen_sol_seq(void) {
    if (debug) printf("RGB: Generating new instruction sequence for round %d.\n", curr_step + 1);
    if (debug) printf("Generated Sequence: ");
    for (int i = 0; i < SOL_LENGTH; i++) {
        curr_sol_seq[i] = (rgb_puzzle_color_t)(rand() % 4); // 0 ~ 3 for R,G,B,Y
        if (debug) printf("%s -> ", get_color_name_str(curr_sol_seq[i]));
    }
    if (debug) printf("\n");
}

// Press LED feedback 
static void set_input_feedback(bool success) {
    color_name_t feedback_color = success ? COLOR_GREEN : COLOR_RED;
    neopixel_set_color(NEO_JEWEL, NEOPIXEL_CENTER_LED, feedback_color);
    app_timer_start(FEEDBACK_SHOW_TIMER, APP_TIMER_TICKS(FEEDBACK_DURATION_MS), NULL); // the clearing work
}

// sequence showing timer handler
static void sol_seq_show_timer_handler(void* unused) {
    if (display_idx < SOL_LENGTH) {        
        // color mapping to neopixel driver
        color_name_t neo_color;
        switch(curr_sol_seq[display_idx]) {
            case PUZZLE_COLOR_RED: neo_color = COLOR_RED; break;
            case PUZZLE_COLOR_GREEN: neo_color = COLOR_GREEN; break;
            case PUZZLE_COLOR_BLUE: neo_color = COLOR_BLUE; break;
            case PUZZLE_COLOR_YELLOW: neo_color = COLOR_YELLOW; break;
            default: neo_color = COLOR_BLACK;
        }

        neopixel_set_color(NEO_JEWEL, NEOPIXEL_OUTER_LED_START + display_idx, neo_color);
        display_idx++;
        app_timer_start(SOL_SEQ_SHOW_TIMER, APP_TIMER_TICKS(SHOW_COLOR_DURATION_MS), NULL);

    } else {
        clear_outer_leds();
        curr_state = WAIT_INPUT;
        input_idx = 0;
    }
}

// Feedback timer handler related to clearing the neopixel jewel LED
static void feedback_timer_handler(void* unused) {
    neopixel_clear(NEO_JEWEL, NEOPIXEL_CENTER_LED);
}

// display the instruction sequence
static void show_sol_seq(void) {
    display_idx = 0;
    clear_outer_leds();
    neopixel_clear(NEO_JEWEL, NEOPIXEL_CENTER_LED);

    curr_state = SHOW_INST;
    //if (debug) printf("RGB: showing instruction\n");
    app_timer_start(SOL_SEQ_SHOW_TIMER, APP_TIMER_TICKS(SHOW_COLOR_DURATION_MS), NULL);
}

// rgb puzzle initialization function (only called once)
void rgb_puzzle_init(uint8_t sx1509_addr, const rgb_puzzle_pins_t* p_pins, bool enable_debug) {
    if (puzzle_initialized) return;

    debug = enable_debug;
    puzzle_pins = p_pins;
    sx1509_i2c_addr = sx1509_addr;

    button_pins[PUZZLE_COLOR_RED] = puzzle_pins->red_button;
    button_pins[PUZZLE_COLOR_GREEN] = puzzle_pins->green_button;
    button_pins[PUZZLE_COLOR_BLUE] = puzzle_pins->blue_button;
    button_pins[PUZZLE_COLOR_YELLOW] = puzzle_pins->yellow_button;

    for (int i = 0; i < 4; i++) {
        //sx1509_pin_config_input_pullup(sx1509_i2c_addr, button_pins[i]);
        nrf_gpio_cfg_input(button_pins[i], NRF_GPIO_PIN_PULLUP);
    }
    nrf_gpio_cfg_input(puzzle_pins->puzzle_select, NRF_GPIO_PIN_PULLUP);
    neopixel_clear_all(NEO_JEWEL);

    app_timer_create(&RGB_PUZZLE_HANDLER_TIMER, APP_TIMER_MODE_REPEATED, rgb_puzzle_handler);
    app_timer_create(&SOL_SEQ_SHOW_TIMER, APP_TIMER_MODE_SINGLE_SHOT, sol_seq_show_timer_handler);
    app_timer_create(&FEEDBACK_SHOW_TIMER, APP_TIMER_MODE_SINGLE_SHOT, feedback_timer_handler);
        
    // Get a random generator seed
    rng_init(); 
    srand(rng_get8());

    for(int i=0; i<4; i++) {
        last_button_pressed_states[i] = false;
    }
    puzzle_initialized = true;
    puzzle_active = false;
    puzzle_complete = false;
    curr_state = IDLE;

    if (debug) printf("RGB: Puzzle Initialized.\n");
}

// Start puzzle (reset state)
void rgb_puzzle_start() {
    if (puzzle_active) return;
    curr_step = 0;
    input_idx = 0;
    puzzle_complete = false;
    puzzle_active = false;
    curr_state = IDLE;
    neopixel_clear_all(NEO_JEWEL);
    if (debug) printf("RGB: Puzzle Start.\n");
}

// End everything related to the puzzle
void rgb_puzzle_stop() {
    if (!puzzle_active) return;

    if (debug) printf("RGB: Puzzle Stop.\n");

    app_timer_stop(RGB_PUZZLE_HANDLER_TIMER);
    app_timer_stop(SOL_SEQ_SHOW_TIMER);
    app_timer_stop(FEEDBACK_SHOW_TIMER);
    
    puzzle_active = false;
    curr_state = IDLE;

    if (puzzle_complete) {
        neopixel_set_color_all(NEO_JEWEL, COLOR_GREEN);
    } else {
        neopixel_clear_all(NEO_JEWEL);
    }
}

// Continue playing existing puzzle instance
void rgb_puzzle_continue(void* unused) {
    if (puzzle_active) return; // flag check (return if already active)
    if (puzzle_complete) return;
    if (debug) printf("RGB: puzzle continue called.\n");
    puzzle_active = true;
    curr_state = START_ROUND; // Kick off the first round
    app_timer_start(RGB_PUZZLE_HANDLER_TIMER, APP_TIMER_TICKS(50), NULL);
}

// Returns true iff puzzle has been successfully completed.
bool rgb_puzzle_is_complete(void) {
    return puzzle_complete;
}

// Returns true iff puzzle is currently active.
bool rgb_puzzle_is_active(void) {
    return puzzle_active;
}

// Process button press event
static void process_button_press(rgb_puzzle_color_t color_pressed) {
    if (curr_state != WAIT_INPUT) return; // wrong state to process input (not likely to happen)
    if (input_idx >= SOL_LENGTH) return;

    color_name_t neo_color; // do the color mapping
    switch(color_pressed) {
        case PUZZLE_COLOR_RED: neo_color = COLOR_RED; break;
        case PUZZLE_COLOR_GREEN: neo_color = COLOR_GREEN; break;
        case PUZZLE_COLOR_BLUE: neo_color = COLOR_BLUE; break;
        case PUZZLE_COLOR_YELLOW: neo_color = COLOR_YELLOW; break;
        default: neo_color = COLOR_BLACK;
    }
    neopixel_set_color(NEO_JEWEL, NEOPIXEL_CENTER_LED, neo_color);

    player_input_seq[input_idx] = color_pressed;
    input_idx++;
    if (debug) printf("RGB: Player input %d/%d: Color %s registered.\n", input_idx, SOL_LENGTH, get_color_name_str(color_pressed));

    if (input_idx == SOL_LENGTH) {
        //if (debug) printf("RGB: Full sequence entered. state -> CHECK_SEQ.\n");
        curr_state = CHECK_SEQ;
    }
}

// Reset the puzzle state
void rgb_puzzle_handler(void* unused) { 
    if (!puzzle_active) return;

    if (curr_step >= NUM_ROUNDS) {
        if (!puzzle_complete) {
             if (debug) printf("RGB: All rounds complete! Puzzle Success!\n");
             puzzle_complete = true;
             // Final success animation is handled in stop()
        }
        rgb_puzzle_stop();
        return;
    }

    // FSM
    switch (curr_state) {
        case IDLE: // default state after initialization & game finished
            // do nothing
            break;

        case START_ROUND:
            if (debug) printf("RGB: State -> START_ROUND (Round %d/%d)\n", curr_step + 1, NUM_ROUNDS);
            gen_sol_seq();
            input_idx = 0;
            show_sol_seq(); // this leads to SHOW_INST
            break;

        case SHOW_INST:
            // do nothing cuz we don't want to read input while showing colors (or there is no memorizing part)
            break;

        case WAIT_INPUT:
            // reading input
            for (int i = 0; i < 4; i++) { // R, G, B, Y
                bool curr_press_state = (nrf_gpio_pin_read(button_pins[i]) == false);

                // same thing we did in keypad. we need debounce or it'll keep spamming input
                if (curr_press_state && !last_button_pressed_states[i]) { // button is released 
                    process_button_press((rgb_puzzle_color_t)i);
                    nrf_delay_ms(BUTTON_DEBOUNCE_DELAY_MS); //  debounce
                }
                last_button_pressed_states[i] = curr_press_state; // update record
            }
            break;

        case CHECK_SEQ:
        {
            //if (debug) printf("RGB: State -> CHECK_SEQ\n");
            bool seq_match = true;
            for (int i = 0; i < SOL_LENGTH; i++) {
                if (player_input_seq[i] != curr_sol_seq[i]) {
                    seq_match = false;
                    break;
                }
            }

            set_input_feedback(seq_match);

            if (seq_match) {
                //if (debug) printf("RGB: input CORRECT!\n");
                curr_state = ROUND_SUCCESS_FEEDBACK;
            } else {
                //if (debug) printf("RGB: input INCORRECT.\n");
                curr_state = ROUND_FAIL_FEEDBACK;
            }
            break;
        }
        case ROUND_SUCCESS_FEEDBACK:
            if (debug) printf("RGB: Round Success.\n");
            curr_step++;
            curr_state = START_ROUND; // go to next round
            break;

        case ROUND_FAIL_FEEDBACK:
            if (debug) printf("RGB: Round Failed. Retrying.\n");
            // go back to start of round
            curr_state = START_ROUND;
            break; 
        default:
            if (debug) printf("RGB: State error: %d. State -> IDLE\n", curr_state);
            curr_state = IDLE;
            break;
    }
}