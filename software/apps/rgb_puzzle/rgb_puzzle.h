#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "neopixel.h"
typedef struct {
    uint8_t red_button;
    uint8_t green_button;
    uint8_t blue_button;
    uint8_t yellow_button;
    uint8_t puzzle_select;
} rgb_puzzle_pins_t;
typedef enum {
    PUZZLE_COLOR_RED,
    PUZZLE_COLOR_YELLOW,
    PUZZLE_COLOR_BLUE,
    PUZZLE_COLOR_GREEN
} rgb_puzzle_color_t;
typedef enum {
    IDLE,
    START_ROUND,
    SHOW_INST,
    WAIT_INPUT,
    CHECK_SEQ,
    ROUND_SUCCESS_FEEDBACK,
    ROUND_FAIL_FEEDBACK,
} rgb_puzzle_fsm_state_t;

void rgb_puzzle_init(uint8_t sx1509_addr, const rgb_puzzle_pins_t* p_pins, bool enable_debug);
void rgb_puzzle_start(void);
void rgb_puzzle_stop(void);
void rgb_puzzle_continue(void* unused);

bool rgb_puzzle_is_complete(void);
bool rgb_puzzle_is_active(void);