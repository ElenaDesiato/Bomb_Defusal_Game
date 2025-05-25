#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#include "nrf_delay.h"
#include "nrf_twi_mngr.h"

#include "microbit_v2.h"
#include "../sx1509/sx1509.h"
#include "../neopixel_ring/neopixel_ring.h"


#define NUM_SWITCHES 5
const uint16_t SWITCH_PIN[NUM_SWITCHES] = {EDGE_P3, EDGE_P4, EDGE_P5, EDGE_P6, EDGE_P7};
uint16_t SWITCH_PIN_MAPPED_VALUE[NUM_SWITCHES] = {0};
bool SWITCH_PIN_STATE[NUM_SWITCHES] = {0};

#define PIN_PUZZLE_SELECT EDGE_P13

// Making randomized mapping for the switches
void randomize_switch_mapping() {
    srand(time(NULL));
    uint16_t target_sum = 0xFFFF; 
    uint16_t sum = 0;

    for (int i = 0; i < NUM_SWITCHES - 1; i++) {
        SWITCH_PIN_MAPPED_VALUE[i] = rand() & 0xFFFF; 
        sum += SWITCH_PIN_MAPPED_VALUE[i]; 
    }
    SWITCH_PIN_MAPPED_VALUE[NUM_SWITCHES - 1] = target_sum - sum;

    for (int i = 0; i < NUM_SWITCHES; i++) {
        SWITCH_PIN_STATE[i] = rand() % 2;
    }

    // Print result
    printf("Mapped Values:\n");
    for (int i = 0; i < NUM_SWITCHES; i++) {
        printf("Switch %d: Mapped Value = %04X, State = %d\n", i, SWITCH_PIN_MAPPED_VALUE[i], SWITCH_PIN_STATE[i]);
    }

    // Verify OR result
    uint16_t or_result = 0;
    for (int i = 0; i < NUM_SWITCHES; i++) {
        or_result |= SWITCH_PIN_MAPPED_VALUE[i];
    }
    printf("Bitwise OR result: %04X\n", or_result);  // Should be FFFF
}

uint16_t sum = 0;
void update_switch_status() {
    sum = 0;
    for (int i = 0; i < NUM_SWITCHES; i++) {
        if (nrf_gpio_pin_read(SWITCH_PIN[i]) == SWITCH_PIN_STATE[i]) {
            sum += SWITCH_PIN_MAPPED_VALUE[i];
        }
    }
    printf("\n"); 
}

void update_neopixel_ring() {   
    for(int i = 0; i < 16; i++) {
        if ((sum >> i) & 1) {
            neopixel_set_color(i, COLOR_RED);
        } else {
            neopixel_clear(i);
        }
    }
}

bool switch_puzzle_clear() {
    if (sum == 0xFFFF) {return true;}
    return false;
}

int main(void) {    
    printf("Board started!\n");
    nrf_gpio_cfg_input(PIN_PUZZLE_SELECT, NRF_GPIO_PIN_PULLUP);
    neopixel_ring_init(9);  

    for (int i = 0; i < NUM_SWITCHES; i++) {
        nrf_gpio_cfg_input(SWITCH_PIN[i], NRF_GPIO_PIN_NOPULL);
    }
    randomize_switch_mapping();
    

    while(1) {
        // Reset if puzzle select is pressed
        if (!nrf_gpio_pin_read(PIN_PUZZLE_SELECT)) {
            printf("RESET button pressed. Puzzle reset.\n");
        }
    
        update_switch_status();
        update_neopixel_ring();
        if (switch_puzzle_clear()){
            neopixel_set_color_all(COLOR_GREEN);
            printf("CLEARED.\n");
            break;
        }
        nrf_delay_ms(400);
    }

    while(1){
        //idle
        }
    return 0;
}