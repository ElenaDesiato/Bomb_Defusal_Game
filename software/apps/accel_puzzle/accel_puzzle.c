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

#include "nrf_delay.h"
#include "nrf_twi_mngr.h"

#include "microbit_v2.h"

#define MAX_INSTRUCTIONS 5  // Maximum number of instructions we can define
#define NUM_INSTRUCTIONS_TO_PLAY 3 // How many instructions to complete the puzzle

#define ANGLE_TOLERANCE 15     // degrees tolerance (increased a bit for usability)
#define HOLD_TIME_MS 2000       // must hold the pose this long
#define CHECK_INTERVAL_MS 200   // check every 200ms (more frequent checks during hold)

APP_TIMER_DEF(ACCEL_CHECK_TIMER); 

NRF_TWI_MNGR_DEF(twi_mngr_instance, 5, 0); 

// Static module variables
static uint8_t sx1509_i2c_addr = 0;
static accel_puzzle_pins_t* puzzle_pins = NULL;
static bool puzzle_initialized = false;
static bool puzzle_active = false; 
static bool puzzle_complete = false;

static bool debug = true; // for debug

// Current state variables
static volatile bool holding_correct_pose_volatile = false; 
static volatile bool accel_check_timer_running = false; 

static accel_puzzle_instruction_t defined_instructions[MAX_INSTRUCTIONS];
static int curr_instruction = 0;
static int instructions_completed_count = 0;

// Forward declarations for static functions
static void stop_accel_check_timer();
static void start_accel_check_timer();
static void posture_check_timer_handler(void* p_context);
static void generate_instruction();
static bool is_angle_in_tolerance(float current_value, float target_value);


static void generate_instruction() {
  // We'll have a set of instructions written beforehand.
  // We can later randomly pick NUM_INSTRUCTIONS_TO_PLAY from these.
  // For now we just use the first N
  defined_instructions[0] = (accel_puzzle_instruction_t){.pitch = 45, .roll = 0};
  defined_instructions[1] = (accel_puzzle_instruction_t){.pitch = 0, .roll = 45};
  defined_instructions[2] = (accel_puzzle_instruction_t){.pitch = -45, .roll = 0};
  defined_instructions[3] = (accel_puzzle_instruction_t){.pitch = 0, .roll = -45};
  defined_instructions[4] = (accel_puzzle_instruction_t){.pitch = 30, .roll = -30};
  // NOTE: angles should be intuitional
  // Pitch: -90 (nose down) to 90 (nose up)
  // Roll: -90 (left side down) to 90 (right side down) - though often -180 to 180. LSM6DSO typically gives good range.
}

// Helper to check if a value is within tolerance of a target
// Handles wrap-around for angles if necessary, but for pitch/roll -90 to +90, simple diff is usually fine.
static bool is_angle_in_tolerance(float current_value, float target_value) {
  return fabs(current_value - target_value) <= ANGLE_TOLERANCE;
}

// Timer handler: Called periodically when the posture hold timer is active
static void posture_check_timer_handler(void* p_context) {
    if (!puzzle_active || !accel_check_timer_running) {
        return;
    }

    float current_pitch_val = 0;
    float current_roll_val = 0;

    if (lsm6dso_is_ready()) {
        current_pitch_val = lsm6dso_get_pitch();
        current_roll_val = lsm6dso_get_roll();
    } else {
        if (debug) printf("ACCEL: LSM6DSO not ready in timer handler.\n");
        holding_correct_pose_volatile = false; 
        return; // Exit if sensor not ready
    }

    accel_puzzle_instruction_t target_instruction = defined_instructions[curr_instruction];

    if (is_angle_in_tolerance(current_pitch_val, target_instruction.pitch) &&
        is_angle_in_tolerance(current_roll_val, target_instruction.roll)) {
        // Still holding correctly, no change to holding_correct_pose_volatile (it should be true)
        if (debug) printf("ACCEL: Timer: Pose OK (P:%.1f, R:%.1f)\n", current_pitch_val, current_roll_val);
    } else {
        if (debug) {
            printf("ACCEL: Timer: Pose LOST! Target (P:%d, R:%d), Actual (P:%.1f, R:%.1f)\n",
                   target_instruction.pitch, target_instruction.roll,
                   current_pitch_val, current_roll_val);
        }
        holding_correct_pose_volatile = false;
        stop_accel_check_timer();
    }
}

static void start_accel_check_timer() {
    holding_correct_pose_volatile = true;
    accel_check_timer_running = true;

    uint32_t err_code = app_timer_start(ACCEL_CHECK_TIMER, APP_TIMER_TICKS(CHECK_INTERVAL_MS), NULL);
    APP_ERROR_CHECK(err_code); 
    if (debug) printf("ACCEL: Posture check timer started.\n");
}

// Stops the posture check timer
static void stop_accel_check_timer() {
    if (accel_check_timer_running) {
        uint32_t err_code = app_timer_stop(ACCEL_CHECK_TIMER);
        // It's okay if it returns NRF_ERROR_INVALID_STATE if timer wasn't running
        if (err_code != NRF_SUCCESS && err_code != NRF_ERROR_INVALID_STATE) {
            APP_ERROR_CHECK(err_code);
        }
        accel_check_timer_running = false;
        if (debug) printf("ACCEL: Posture check timer stopped.\n");
    }
    // `holding_correct_pose_volatile` is NOT reset here. Its value after the hold period is what matters.
}

// Public function to initialize the accel puzzle module
void accel_puzzle_init(uint8_t sx1509_addr, accel_puzzle_pins_t* p_pins, bool enable_debug) {
    // pins
    sx1509_i2c_addr = sx1509_addr;
    puzzle_pins = p_pins;
    debug = enable_debug;

    // i2c config & initialization
    nrf_drv_twi_config_t i2c_config = NRF_DRV_TWI_DEFAULT_CONFIG;
    i2c_config.scl = EDGE_P19;
    i2c_config.sda = EDGE_P20;
    i2c_config.frequency = NRF_DRV_TWI_FREQ_100K;
    i2c_config.interrupt_priority = 0;
    nrf_twi_mngr_init(&twi_mngr_instance, &i2c_config);

    sx1509_init(sx1509_i2c_addr,  &twi_mngr_instance);
    if (debug) printf("SX1509 is %s\n", sx1509_is_connected(sx1509_i2c_addr,  &twi_mngr_instance) ? "connected" : "not connected");

    // Voice Synth init
    DFR0760_init(&twi_mngr_instance);

    if (DFR0760_is_connected(&twi_mngr_instance)){
       printf("DFR0760 is connected.\n");
    } else {
       printf("DFR0760 is NOT connected.\n");
    }
    nrf_delay_ms(100); 
    DFR0760_set_volume(1);


    app_timer_init(); 

    // Initialize accelerometer
    lsm6dso_init(&twi_mngr_instance);
    if (debug) printf("ACCEL: LSM6DSO initialized.\n");

    // Create the app timer
    uint32_t err_code = app_timer_create(&ACCEL_CHECK_TIMER, APP_TIMER_MODE_REPEATED, posture_check_timer_handler);
    APP_ERROR_CHECK(err_code);

    generate_instruction();
    curr_instruction = 0;
    instructions_completed_count = 0;
    puzzle_initialized = true;
    puzzle_active = false; // Puzzle is initialized but not yet started

    if (debug) printf("ACCEL: Puzzle module initialized.\n");
}


bool accel_puzzle_start(void) {
    DFR0760_say("Starting accelerometer puzzle.");
    nrf_delay_ms(500);

    puzzle_active = true;
    curr_instruction = 0; // Start from the first instruction
    instructions_completed_count = 0;

    while (instructions_completed_count < NUM_INSTRUCTIONS_TO_PLAY && puzzle_active) {
        if (curr_instruction >= MAX_INSTRUCTIONS) {
            if (debug) printf("ACCEL: Ran out of defined instructions!\n");
            DFR0760_say("Internal error.");
            puzzle_active = false;
            return false;
        }

        accel_puzzle_instruction_t current_target = defined_instructions[curr_instruction];
        char speech_buf[128];
        snprintf(speech_buf, sizeof(speech_buf), "Tilt pitch to %d and roll to %d degrees.",
                 current_target.pitch, current_target.roll);
        DFR0760_say(speech_buf);

        if (debug) {
            printf("ACCEL: Instruction %d: Target Pitch=%d, Roll=%d\n",
                   instructions_completed_count + 1, current_target.pitch, current_target.roll);
        }

        bool current_instruction_done = false;
        while (!current_instruction_done && puzzle_active) {
            float live_pitch = 0;
            float live_roll = 0;
            if (lsm6dso_is_ready()) {
                live_pitch = lsm6dso_get_pitch();
                live_roll = lsm6dso_get_roll();
                if (debug && !accel_check_timer_running) { // Only print live data when not in timed hold
                     printf("ACCEL: Live P:%.1f, R:%.1f (Target P:%d, R:%d)\r",
                           live_pitch, live_roll, current_target.pitch, current_target.roll);
                }
            } else {
                if (debug) printf("ACCEL: LSM6DSO not ready in main loop.\n");
                nrf_delay_ms(200); 
                continue;
            }

            if (is_angle_in_tolerance(live_pitch, current_target.pitch) &&
                is_angle_in_tolerance(live_roll, current_target.roll)) {

                if (debug) printf("\nACCEL: Pose DETECTED! Starting hold timer...\n");
                DFR0760_say("Hold it.");
                nrf_delay_ms(200); // Small delay to ensure user is stable after speech

                start_accel_check_timer();
                nrf_delay_ms(HOLD_TIME_MS); // Blocking delay for the hold duration
                stop_accel_check_timer();  // Stop the timer regardless of outcome

                if (holding_correct_pose_volatile) {
                    if (debug) printf("ACCEL: Pose HELD successfully for step %d!\n", instructions_completed_count + 1);
                    DFR0760_say("Good.");
                    nrf_delay_ms(300);
                    current_instruction_done = true;
                    instructions_completed_count++;
                    curr_instruction++; // Move to next DIFFERENT instruction in the defined list
                                               // Or implement random selection here
                } else {
                    if (debug) printf("ACCEL: Pose NOT HELD. Try again for P:%d, R:%d.\n", current_target.pitch, current_target.roll);
                    DFR0760_say("Try again.");
                    // Loop continues for the same instruction
                }
            }
            nrf_delay_ms(100); // Polling delay when not in tolerance / not in hold period
        } // End while !current_instruction_done
    } // End while instructions_completed_count < NUM_INSTRUCTIONS_TO_PLAY

    if (puzzle_active && instructions_completed_count >= NUM_INSTRUCTIONS_TO_PLAY) {
        if (debug) printf("ACCEL: Puzzle COMPLETED!\n");
        DFR0760_say("Accelerometer puzzle complete. Well done!");
        puzzle_active = false;
        return true;
    } else {
        // Puzzle was likely cancelled or an error occurred
        if (debug && puzzle_active) printf("ACCEL: Puzzle exited prematurely.\n");
        // DFR0760_say("Puzzle stopped."); // Avoid saying this if already cancelled by user
        puzzle_active = false;
        return false;
    }
}

bool is_accel_puzzle_active(void) {
    return puzzle_active;
}