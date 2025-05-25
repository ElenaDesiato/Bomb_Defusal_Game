#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>

#include "nrf_delay.h"
#include "microbit_v2.h"
#include "lilybuzzer.h"

int main(void) {
  printf("Board started!\n");
  lilybuzzer_init();  

  while (1) {
    play_morse_message("059", 3); 
    nrf_delay_ms(10000); 
  }
}