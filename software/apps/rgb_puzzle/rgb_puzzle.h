#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint8_t red_button;
    uint8_t yellow_button;
    uint8_t green_button;
    uint8_t blue_button;

    uint8_t puzzle_select;
    uint8_t neopixel_jewel;
} rgb_puzzle_pins_t; 

typedef enum {
    R = 0,
    G = 1,
    B = 2,
    Y = 3,
} rgb_puzzle_color_t;

// Puzzle Finite State Machine (FSM) states
typedef enum {
    PUZZLE_STATE_IDLE,
    PUZZLE_STATE_START_ROUND,
    PUZZLE_STATE_SHOWING_INSTRUCTION,
    PUZZLE_STATE_WAITING_FOR_INPUT,
    PUZZLE_STATE_CHECKING_SEQUENCE,      // Transient state after input is complete
    PUZZLE_STATE_ROUND_SUCCESS_FEEDBACK,
    PUZZLE_STATE_ROUND_FAILURE_FEEDBACK,
    PUZZLE_STATE_GAME_OVER_SUCCESS,      // Final success state
} rgb_puzzle_fsm_state_t;
