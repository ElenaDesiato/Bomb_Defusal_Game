#include "microbit_v2.h"
#include "switch_puzzle.h"
#include "nrf_delay.h"
#include "app_timer.h"
#include "../neopixel/neopixel.h"

static const neopixel_pins_t neopixel_pins = {
  .ring = EDGE_P9,
  .jewel = EDGE_P2,
  .stick = EDGE_P1,
};

static bool debug = true;

static const switch_puzzle_pins_t switch_puzzle_pins = {
    // All on breakout
    .switches = {EDGE_P3, EDGE_P4, EDGE_P5, EDGE_P6, EDGE_P7},
    .puzzle_select = EDGE_P14,
};

int main(void) {
    app_timer_init(); 
    neopixel_init(&neopixel_pins, debug);
    switch_puzzle_init(&switch_puzzle_pins, debug); 
    switch_puzzle_start();
    bool started = false; 
    while (1) {
        if (!started && !nrf_gpio_pin_read(switch_puzzle_pins.puzzle_select)) {
            switch_puzzle_continue(NULL); 
            started = true;
        }
        nrf_delay_ms(100);
    }
}