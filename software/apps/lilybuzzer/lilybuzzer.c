// Lily_Buzzer Driver for Microbit_v2
// Basic code structure reference: Lab5 pwm_square_tone
// Use PWM signals to play sounds using buzzer

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "nrf.h"
#include "nrf_delay.h"
#include "nrfx_pwm.h"
#include "app_timer.h"

#include "microbit_v2.h"
#include "lilybuzzer.h"

// Based on international timing of morse code: https://morsecode.world/international/timing.html
#define TIME_UNIT_MS 120                              // one time unit in ms   
#define DOT_LENGTH (1*TIME_UNIT_MS)
#define DASH_LENGTH (3*TIME_UNIT_MS)
#define DOT_DASH_SPACE_LENGTH (1*TIME_UNIT_MS)        // space between dots & dashes of a single character
#define CHAR_SPACE_LENGTH (3*TIME_UNIT_MS)            // space between characters
#define WORD_SPACE_LENGTH (7*TIME_UNIT_MS)            // space between words    

#define MORSE_NOTE_FREQUENCY 700                      // Frequency used in beeps for morse code

APP_TIMER_DEF(symbol_timer);

// One morse symbol
typedef enum {
    DOT,
    DASH, 
    PAUSE
} symbol_t; 

// A struct to map a character to a morse code pattern
typedef struct {
    char c; 
    symbol_t pattern[9];
    uint8_t length;   
} morse_map_t;

// Morse code table
static const morse_map_t morse_table[] = {
    {'A', {DOT, PAUSE, DASH}, 3},
    {'B', {DASH, PAUSE, DOT, PAUSE, DOT, PAUSE, DOT}, 7},
    {'C', {DASH, PAUSE, DOT, PAUSE, DASH, PAUSE, DOT}, 7},
    {'D', {DASH, PAUSE, DOT, PAUSE, DOT}, 5},
    {'E', {DOT}, 1},
    {'F', {DOT, PAUSE, DOT, PAUSE, DASH, PAUSE, DOT}, 7},
    {'G', {DASH, PAUSE, DASH, PAUSE, DOT}, 5},
    {'H', {DOT, PAUSE, DOT, PAUSE, DOT, PAUSE, DOT}, 7},
    {'I', {DOT, PAUSE, DOT}, 3},
    {'J', {DOT, PAUSE, DASH, PAUSE, DASH, PAUSE, DASH}, 7},
    {'K', {DASH, PAUSE, DOT, PAUSE, DASH}, 5},
    {'L', {DOT, PAUSE, DASH, PAUSE, DOT, PAUSE, DOT}, 7},
    {'M', {DASH, PAUSE, DASH}, 3},
    {'N', {DASH, PAUSE, DOT}, 3},
    {'O', {DASH, PAUSE, DASH, PAUSE, DASH}, 5},
    {'P', {DOT, PAUSE, DASH, PAUSE, DASH, PAUSE, DOT}, 7},
    {'Q', {DASH, PAUSE, DASH, PAUSE, DOT, PAUSE, DASH}, 7},
    {'R', {DOT, PAUSE, DASH, PAUSE, DOT}, 5},
    {'S', {DOT, PAUSE, DOT, PAUSE, DOT}, 5},
    {'T', {DASH}, 1},
    {'U', {DOT, PAUSE, DOT, PAUSE, DASH}, 5},
    {'V', {DOT, PAUSE, DOT, PAUSE, DOT, PAUSE, DASH}, 7},
    {'W', {DOT, PAUSE, DASH, PAUSE, DASH}, 5},
    {'X', {DASH, PAUSE, DOT, PAUSE, DOT, PAUSE, DASH}, 7},
    {'Y', {DASH, PAUSE, DOT, PAUSE, DASH, PAUSE, DASH}, 7},
    {'Z', {DASH, PAUSE, DASH, PAUSE, DOT, PAUSE, DOT}, 7},
    {'0', {DASH, PAUSE, DASH, PAUSE, DASH, PAUSE, DASH, PAUSE, DASH}, 9},
    {'1', {DOT, PAUSE, DASH, PAUSE, DASH, PAUSE, DASH, PAUSE, DASH}, 9},
    {'2', {DOT, PAUSE, DOT, PAUSE, DASH, PAUSE, DASH, PAUSE, DASH}, 9},
    {'3', {DOT, PAUSE, DOT, PAUSE, DOT, PAUSE, DASH, PAUSE, DASH}, 9},
    {'4', {DOT, PAUSE, DOT, PAUSE, DOT, PAUSE, DOT, PAUSE, DASH}, 9},
    {'5', {DOT, PAUSE, DOT, PAUSE, DOT, PAUSE, DOT, PAUSE, DOT}, 9},
    {'6', {DASH, PAUSE, DOT, PAUSE, DOT, PAUSE, DOT, PAUSE, DOT}, 9},
    {'7', {DASH, PAUSE, DASH, PAUSE, DOT, PAUSE, DOT, PAUSE, DOT}, 9},
    {'8', {DASH, PAUSE, DASH, PAUSE, DASH, PAUSE, DOT, PAUSE, DOT}, 9},
    {'9', {DASH, PAUSE, DASH, PAUSE, DASH, PAUSE, DASH, PAUSE, DOT}, 9},
};

// PWM configuration
static const nrfx_pwm_t PWM_INST = NRFX_PWM_INSTANCE(0);

// Holds duty cycle values to trigger PWM toggle
static nrf_pwm_values_common_t sequence_data[1] = {0};

// Sequence structure for configuring DMA
static nrf_pwm_sequence_t pwm_sequence = {
  .values.p_common = sequence_data,
  .length = 1,
  .repeats = 0,
  .end_delay = 0,
};

// global variables to hold current morse state
static char* string_to_play = ""; 
static uint32_t string_length = 0; 
static uint32_t curr_char_index = 0; 
static uint8_t curr_symbol_index = 0; 

static uint8_t lilybuzzer_pin = 0;

static void play_curr_symbol();

void lilybuzzer_init(uint8_t buzzer_pin) {
    lilybuzzer_pin = buzzer_pin;

    nrfx_pwm_config_t config = {
        .output_pins = { lilybuzzer_pin, NRFX_PWM_PIN_NOT_USED, NRFX_PWM_PIN_NOT_USED, NRFX_PWM_PIN_NOT_USED},
        .irq_priority = 1,
        .base_clock = NRF_PWM_CLK_500kHz,
        .count_mode = NRF_PWM_MODE_UP,
        .top_value = 0,
        .load_mode = NRF_PWM_LOAD_COMMON,
        .step_mode = NRF_PWM_STEP_AUTO,
    }; 
    nrfx_pwm_init(&PWM_INST, &config, NULL);

    NRF_PWM0->COUNTERTOP = 500000/MORSE_NOTE_FREQUENCY;
    sequence_data[0]= 0.5 * (500000/MORSE_NOTE_FREQUENCY); 

    //app_timer_init(); 
    app_timer_create(&symbol_timer, APP_TIMER_MODE_SINGLE_SHOT, play_curr_symbol);

    string_to_play = ""; 
    string_length = 0;
    curr_char_index = 0; 
    curr_symbol_index = 0; 
    }

// Stop playing all sounds
void stop_buzzer(void) {
    nrfx_pwm_stop(&PWM_INST, false);
    app_timer_stop(symbol_timer); 
}

/* Helper function: lookup character in morse code table by iterating through entire table
    - only defined uppercase letters, so any lowercase letters are converted
   Is very inefficient but it might be sufficient. 
*/
static morse_map_t* morse_lookup(char ch) {
    if (ch >= 'a' && ch <= 'z') ch -= 32; // convert to upper case
    if (ch >= 'A' && ch <= 'Z') {
        return morse_table + ch - 65;  // this is essentially an array access but without the dereference
    }
    if (ch >= '0' && ch <= '9') {
        return morse_table + ch - 48 + 26; 
    }
    return NULL; 
}

/* Callback function called when timer is triggered
*/
static void play_curr_symbol() {
    nrfx_pwm_stop(&PWM_INST, true);
    if (curr_char_index >= string_length) {
        stop_buzzer(); 
        return; 
    }
    uint32_t timer_ms = 0; 
    morse_map_t* curr_char = morse_lookup(string_to_play[curr_char_index]); 
    if (curr_symbol_index >= curr_char->length) { // pattern of current char is over
        timer_ms = CHAR_SPACE_LENGTH;
        curr_char_index++; 
        curr_symbol_index = 0;
    }
    else {
        symbol_t curr_symbol = curr_char->pattern[curr_symbol_index]; 
        if (curr_symbol == DOT) {
            timer_ms = DOT_LENGTH;
            nrfx_pwm_simple_playback(&PWM_INST, &pwm_sequence,1, NRFX_PWM_FLAG_LOOP); 
        } 
        else if (curr_symbol == DASH) {
            timer_ms = DASH_LENGTH;
            nrfx_pwm_simple_playback(&PWM_INST, &pwm_sequence,1, NRFX_PWM_FLAG_LOOP); 
        }
        else { // PAUSE
            timer_ms = DOT_DASH_SPACE_LENGTH;
        }
        curr_symbol_index++; 
        
    }
    app_timer_start(symbol_timer, 33*timer_ms, NULL); //33 ticks = roughly 1 ms
}


/* Play given message in morse code 
    - morse code details defined in macros based on international standards
*/
void play_morse_message(const char* message, uint32_t length) {
    string_to_play = message; 
    string_length = length;
    curr_char_index = 0; 
    curr_symbol_index = 0; 
    play_curr_symbol(); 
}





