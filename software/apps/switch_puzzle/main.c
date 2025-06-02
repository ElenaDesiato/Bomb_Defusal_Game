#include "microbit_v2.h"
#include "switch_puzzle.h"

int main(void) {
    switch_puzzle_pins_t switch_puzzle_pins = {
        // All on breakout
        .switches = {EDGE_P3, EDGE_P4, EDGE_P5, EDGE_P6, EDGE_P7},
        .puzzle_select = EDGE_P14,
        .neopixel = EDGE_P9,
    };
    switch_puzzle_init(&switch_puzzle_pins, false); 
    while (1) {
        if (!nrf_gpio_pin_read(switch_puzzle_pins.puzzle_select)) {
            switch_puzzle_start(); 
        }
    }
}