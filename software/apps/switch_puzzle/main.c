#include "microbit_v2.h"
#include "switch_puzzle.h"

int main(void) {
    printf("Board started ...\n");
    uint16_t switch_pins[NUM_SWITCHES] = {EDGE_P3, EDGE_P4, EDGE_P5, EDGE_P6, EDGE_P7};
    uint16_t puzzle_select_pin = EDGE_P13; 
    uint16_t neopixel_pin = EDGE_P9;
    switch_puzzle_init(puzzle_select_pin, switch_pins, neopixel_pin); 
    while (1) {
        if (!nrf_gpio_pin_read(puzzle_select_pin)) {
            switch_puzzle_start(); 
        }
    }
}