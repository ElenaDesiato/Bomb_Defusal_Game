/* Random number generator, copied from lab1
*/

#include "rng.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

typedef struct {
  uint32_t TASKS_START; 
  uint32_t TASKS_STOP; 
  uint32_t unused_space_1[(0x100 - 0x004)/4 - 1];
  uint32_t EVENTS_VALRDY;
  uint32_t unused_space_2[(0x200 - 0x100)/4 - 1];
  uint32_t SHORTS;
  uint32_t unused_space_3[(0x304 - 0x200)/4 - 1];
  uint32_t INTENSET; 
  uint32_t INTENCLR; 
  uint32_t unused_space_4[(0x504 - 0x308)/4 - 1];
  uint32_t CONFIG; 
  uint32_t VALUE; 
} rng_reg_t;

static volatile rng_reg_t* RNG = (rng_reg_t*) 0x4000D000; 

// Initializes the RNG
// Enable bias correction
// Stops itself after generating a value
void rng_init(void) {
  RNG->CONFIG = 1; // enable bias correction
}

// Get a random 8-bit value
// Waits until a value is ready
uint8_t rng_get8(void) {
  // TODO: implement me
  // HINT: be sure to clear EVENTS_VALRDY before starting the RNG
  RNG->EVENTS_VALRDY = 0; 
  RNG->TASKS_START = 1; 
  volatile uint32_t ready = RNG->EVENTS_VALRDY; 
  while(!ready){
    ready = RNG->EVENTS_VALRDY; 
  }
  volatile uint8_t val = (uint8_t)RNG->VALUE; 
  return val;
}
