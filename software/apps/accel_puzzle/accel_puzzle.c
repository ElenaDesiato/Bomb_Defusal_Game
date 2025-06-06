#include "accel_puzzle.h"
#include "app_timer.h"
#include "sx1509.h"
#include "DFR0760.h"
#include "LSM6DSO.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "nrf_log.h"
#include "app_error.h"
#include "../neopixel/neopixel.h"

#include "nrf_delay.h"
#include "nrf_twi_mngr.h"
#include "microbit_v2.h"

#define MAX_INSTRUCTIONS 5
#define NUM_INSTRUCTIONS_TO_PLAY 3

#define ANGLE_TOLERANCE 20
#define HOLD_TIME_MS 2000  // how long the pose needs to be held (ms)
#define CHECK_INTERVAL_MS 200 // the check interval during hold check (ms)

#define DEBUG_PRINT_INTERVAL_MS 1000 // used for the sophisticated debug print

APP_TIMER_DEF(ACCEL_GAME_HANDLER_TIMER);
APP_TIMER_DEF(ACCEL_CHECK_TIMER);
APP_TIMER_DEF(HOLD_FEEDBACK_TIMER); 
APP_TIMER_DEF(HOLD_FAIL_TIMER); // for flashing red on NEOstick

static uint8_t sx1509_i2c_addr = 0;
static const accel_puzzle_pins_t* puzzle_pins;
static bool debug = true;

static bool puzzle_initialized = false;
static bool puzzle_active = false;
static bool puzzle_complete = false;

static volatile bool pose_is_held = false;
static volatile bool hold_timer_running = false;

static bool step_announced = false;
static bool timing_hold = false;
static uint32_t hold_start_ticks = 0;

static uint32_t last_debug_print_ticks = 0;

static accel_puzzle_instruction_t defined_instructions[MAX_INSTRUCTIONS];
static int current_step = 0;
static int steps_done = 0; // aka number of instructions completed

static uint8_t hold_led_index = 0;

void accel_puzzle_handler(void* unused);

static void hold_feedback_handler(void *unused) {
    if (hold_led_index < 8) {
        neopixel_set_color(NEO_STICK, hold_led_index, COLOR_GREEN);
        hold_led_index++;
    } else {
        // All LEDs lit, stop timer
        app_timer_stop(HOLD_FEEDBACK_TIMER);
        neopixel_clear_all(NEO_STICK);
        hold_led_index = 0; // Reset for next use
    }
}

void hold_fail_handler(void *unused) {
    app_timer_stop(HOLD_FAIL_TIMER);
    hold_led_index = 0;
    neopixel_clear_all(NEO_STICK);
}

static void setup_instructions() {
  defined_instructions[0] = (accel_puzzle_instruction_t){.pitch = 45, .roll = 0};
  defined_instructions[1] = (accel_puzzle_instruction_t){.pitch = 0, .roll = 45};
  defined_instructions[2] = (accel_puzzle_instruction_t){.pitch = -45, .roll = 0};
  defined_instructions[3] = (accel_puzzle_instruction_t){.pitch = 0, .roll = -45};
  defined_instructions[4] = (accel_puzzle_instruction_t){.pitch = 30, .roll = -30};
  // TODO: add more instructions
}

static bool is_angle_in_tolerance(float current_value, float target_value) {
  return fabs(current_value - target_value) <= ANGLE_TOLERANCE; // consider wrap around i guess
}


// FYI: after finishing the code, I realized that my naming scheme is kinda poor and might be confusing
// so I will try to explain it here:
//
// hold_check : check if current pose is held in the target angles
// if we detected that the pose is held, we set pose_is_held to true & start the hold timer (ACCEL_CHECK_TIMER)

static void hold_check_handler(void* unused) {
    if (!puzzle_active || !hold_timer_running) return;
    if (!lsm6dso_is_ready()) {
        pose_is_held = false;
        return;
    }

    float pitch = lsm6dso_get_pitch();
    float roll = lsm6dso_get_roll();
    accel_puzzle_instruction_t target = defined_instructions[current_step];

    if (!is_angle_in_tolerance(pitch, target.pitch) || !is_angle_in_tolerance(roll, target.roll)) {
        pose_is_held = false;
    }
}

static void start_hold_timer() {
    pose_is_held = true;
    hold_timer_running = true;
    app_timer_start(ACCEL_CHECK_TIMER, APP_TIMER_TICKS(CHECK_INTERVAL_MS), NULL);
    app_timer_start(HOLD_FEEDBACK_TIMER, APP_TIMER_TICKS(250), NULL);
}

static void stop_hold_timer() {
    if (hold_timer_running) {
        app_timer_stop(ACCEL_CHECK_TIMER);
        hold_timer_running = false;
    }
}

static void reset_step() {
    stop_hold_timer();
    timing_hold = false;
    step_announced = false;
}

static void reset_puzzle() {
    if (debug) printf("ACCEL: Puzzle reset.\n");
    steps_done = 0;
    current_step = 0;
    puzzle_complete = false;
    neopixel_clear_all(NEO_STICK);
    reset_step();
}

void accel_puzzle_init(uint8_t i2c_addr, const nrf_twi_mngr_t* twi_mgr_instance, const accel_puzzle_pins_t* p_pins, bool enable_debug) {
    if (puzzle_initialized) return;

    sx1509_i2c_addr = i2c_addr;
    puzzle_pins = p_pins;
    debug = enable_debug;

    sx1509_pin_config_input_pullup(sx1509_i2c_addr, p_pins->puzzle_select);

    lsm6dso_init(twi_mgr_instance);

    app_timer_create(&ACCEL_GAME_HANDLER_TIMER, APP_TIMER_MODE_REPEATED, accel_puzzle_handler);
    app_timer_create(&ACCEL_CHECK_TIMER, APP_TIMER_MODE_REPEATED, hold_check_handler);
    app_timer_create(&HOLD_FEEDBACK_TIMER, APP_TIMER_MODE_REPEATED, hold_feedback_handler);
    app_timer_create(&HOLD_FAIL_TIMER, APP_TIMER_MODE_SINGLE_SHOT, hold_fail_handler);
    setup_instructions();
    puzzle_initialized = true;
    neopixel_clear_all(NEO_STICK);
    if (debug) printf("ACCEL: Puzzle module initialized.\n");
}

bool accel_puzzle_start(void) {
    if (puzzle_active) return false; // game is already running, why start
    reset_puzzle();
    lsm6dso_start(); 

    if (debug) printf("ACCEL: Puzzle started.\n");
    return true;
}

void accel_puzzle_stop(void) {
    if (!puzzle_active) return;

    if (debug) printf("ACCEL: Puzzle stopped.\n");
    puzzle_active = false;
    reset_step();
    app_timer_stop(ACCEL_GAME_HANDLER_TIMER);
    app_timer_stop(HOLD_FEEDBACK_TIMER);
    app_timer_stop(HOLD_FAIL_TIMER);
    lsm6dso_stop(); 
    if (puzzle_complete) {
        neopixel_set_color_all(NEO_STICK, COLOR_GREEN);
    }
    else {
        neopixel_clear_all(NEO_STICK);
        hold_led_index = 0;
    }
}

void accel_puzzle_handler(void* unused) {

    if (steps_done >= NUM_INSTRUCTIONS_TO_PLAY) {
            if (!puzzle_complete) {
                if (debug) printf("\n");
                if (debug) printf("ACCEL: Puzzle COMPLETED!\n");
                DFR0760_say("Accelerometer puzzle complete. Well done!");
                puzzle_complete = true;
            }
            accel_puzzle_stop();
            return;
        }

    accel_puzzle_instruction_t target = defined_instructions[current_step];

    if (!step_announced) {
        char speech_buf[128];
        snprintf(speech_buf, sizeof(speech_buf), "x equals to %d. y equals to %d.", target.pitch, target.roll);
        DFR0760_say(speech_buf);
        if (debug) printf("ACCEL: Instruction %d: Target P:%d, R:%d\n", steps_done + 1, target.pitch, target.roll);
        step_announced = true;
        return;
    }

    // Explaination: 
    //   timing_hold = false -> check the angles (hold check) until they enter the tolerance range of the target angles
    //                          then we set timing_hold to true & start the hold timer 
    //   timing_hold = true  -> check if the angles are still in the tolerance range
    
    // Not into the hold yet
    if (!timing_hold) {
        if (!lsm6dso_is_ready()) return;  // I'm thinking about whether we need to decrease the ACC_MEASUREMENT_INTERVAL for LSM6DSO

        float live_pitch = lsm6dso_get_pitch();
        float live_roll = lsm6dso_get_roll();

        // Super sophisticated debug print, idk how it works but it does
        // Source: chatGPT
        if (debug) {
            uint32_t current_ticks, diff_ticks;
            current_ticks = app_timer_cnt_get();
            diff_ticks = app_timer_cnt_diff_compute(current_ticks, last_debug_print_ticks);

            if (diff_ticks >= APP_TIMER_TICKS(DEBUG_PRINT_INTERVAL_MS)) {
                printf("ACCEL_DBG: Live P:%6.1f, R:%6.1f \r", live_pitch, live_roll);
                last_debug_print_ticks = current_ticks;
            }
        }

        // check if pitch & roll angles hold
        if (is_angle_in_tolerance(live_pitch, target.pitch) && is_angle_in_tolerance(live_roll, target.roll)) {
            if (debug) printf("\n");
            //DFR0760_say("Hold it.");
            start_hold_timer();
            hold_start_ticks = app_timer_cnt_get();  // record the start time of the hold
            timing_hold = true;
        }
        return;
    }

    // CHECKING the hold DURATION
    if (timing_hold) {
        if (!pose_is_held) {
            if (debug) printf("\n");
            //DFR0760_say("Try again.");
            app_timer_stop(HOLD_FEEDBACK_TIMER);
            app_timer_start(HOLD_FAIL_TIMER, APP_TIMER_TICKS(500), NULL);
            neopixel_set_color_all(NEO_STICK, COLOR_RED);
            reset_step();
            return;
        }

        // calculate how many ticks have passed since the hold started
        uint32_t current_ticks, elapsed_ticks;
        current_ticks = app_timer_cnt_get();
        elapsed_ticks = app_timer_cnt_diff_compute(current_ticks, hold_start_ticks);

        if (elapsed_ticks >= APP_TIMER_TICKS(HOLD_TIME_MS)) {
            if (debug) printf("\n");
            neopixel_clear_all(NEO_STICK);
            DFR0760_say("Good.");
            steps_done++;
            current_step++;
            reset_step();
        }
    }
}

void accel_puzzle_continue(void* unused) {
    if (puzzle_active) { // preventing firing the handler timer multiple times
        return;
    }
    lsm6dso_start(); 
    puzzle_active = true;
    app_timer_start(ACCEL_GAME_HANDLER_TIMER, APP_TIMER_TICKS(100), NULL);
}

bool accel_puzzle_is_active(void) {
    return puzzle_active;
}

bool accel_puzzle_is_complete(void) {
    return puzzle_complete;
}