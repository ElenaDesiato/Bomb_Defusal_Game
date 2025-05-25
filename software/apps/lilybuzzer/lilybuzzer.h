#ifndef LILYBUZZER_H
#define LILYBUZZER_H

#include <stdint.h>

// Initialize the lily_buzzer sensor driver
void lilybuzzer_init();

// Stop all sounds from buzzer
void stop_buzzer();

/* Play message in morse code
    - length must be equal to the number of characters in message
    - no support for spaces/different words
*/
void play_morse_message(const char *message, uint32_t length); 

#endif