// NeoPixel Ring Driver (16LED) for Microbit_v2
// Code Framework reference: Lab5 PWM-SquareWaveTone & Slides about NeoPixel ring
// Use PWM signals to drive the NeoPixel Ring

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "nrf.h"
#include "nrf_delay.h"
#include "nrfx_pwm.h"

#include "microbit_v2.h"
#include "neopixel_ring.h"

#define NEOPIXEL_PIN (EDGE_P7)

#define ONE_HIGH 13
#define ZERO_HIGH 6
#define RESET_DELAY_US 100

static struct {
  uint8_t r, g, b;
} rgb[16]; // 16 LEDs

//RGB
// By not having full 255 values (instead ~1/4 of that) the LEDs should not be bright enough to hurt our eyes
static const color_t COLOR_TABLE[COLOR_COUNT] = {
  [COLOR_BLACK]   = {0, 0, 0},
  [COLOR_WHITE]   = {64, 64, 64},
  [COLOR_RED]     = {64, 0, 0},
  [COLOR_GREEN]   = {0, 64, 0},
  [COLOR_BLUE]    = {0, 0, 64},
  [COLOR_YELLOW]  = {64, 64, 0},
  [COLOR_CYAN]    = {0, 64, 64},
  [COLOR_MAGENTA] = {64, 0, 64},
  [COLOR_ORANGE]  = {64, 32, 0},
  [COLOR_PURPLE]  = {32, 0, 64},
  [COLOR_PINK]    = {64, 26, 45},
};

// PWM configuration
static const nrfx_pwm_t PWM_INST = NRFX_PWM_INSTANCE(0);

// Holds duty cycle values to trigger PWM toggle
static nrf_pwm_values_common_t sequence_data[16 * 24] = {0};

// Sequence structure for configuring DMA
nrf_pwm_sequence_t pwm_sequence = {
  .values.p_common = sequence_data,
  .length = 1,
  .repeats = 0,
  .end_delay = 0,
};

static void print_sequence_data() {
  int bits_per_led = 24;
  int led_count = 16; 
  int total_bits = led_count * bits_per_led;
  printf("sequence_data (first %d LEDs):\n", led_count);
  for (int i = 0; i < total_bits; i++) {
      printf("%2d ", sequence_data[i]);
      if ((i + 1) % bits_per_led == 0) {
          printf("\n");
      }
  }
  printf("\n");
}

void neopixel_ring_init(uint8_t pin) {
  nrf_gpio_cfg_output(NEOPIXEL_PIN);

  // Initialize the PWM
  nrfx_pwm_config_t config = {
    .output_pins = {
        NEOPIXEL_PIN,                
        NRFX_PWM_PIN_NOT_USED,    
        NRFX_PWM_PIN_NOT_USED,    
        NRFX_PWM_PIN_NOT_USED     
    },
    .irq_priority = NRFX_PWM_DEFAULT_CONFIG_IRQ_PRIORITY,
    .base_clock = NRF_PWM_CLK_16MHz,
    .count_mode = NRF_PWM_MODE_UP,
    .top_value = 20,
    .load_mode = NRF_PWM_LOAD_COMMON,
    .step_mode = NRF_PWM_STEP_AUTO
  };
  nrfx_pwm_init(&PWM_INST, &config, NULL);
  neopixel_clear_all();
}

void neopixel_set_rgb(uint8_t led,uint8_t r, uint8_t g, uint8_t b) {
    rgb[led].r = r;
    rgb[led].g = g;
    rgb[led].b = b;
}

/* Specific numbers are taken from: https://wp.josh.com/2014/05/13/ws2812-neopixels-are-not-so-finicky-once-you-get-to-know-them/
  - timer frequency: 16 MHz
  - desired PWM signal frequency: 800 KHz - period is 1.2us
  - each tick is then 1/16 Mhz = 62.5s
  - duty cycle for 0: 6 (high time: 375ns - fits neopixel)
  - duty cycle for 1: 14 (high time: 875ns - fits neopixel)
  - reset delay: at least 50 us
  Computation similar to lab 5: 
  - countertop = input clock/output frequency  = 16 000 000 / 800 000 = 20
*/

void neopixel_show(void) {
  uint16_t pwm_index = 0;

  // Make sure that data is sent in order GRB and then with bit 7 first
  for (int i = 0; i < 16; i++) {
    uint8_t rgb_data_seq[3] = { rgb[i].g, rgb[i].r, rgb[i].b };

    for (int j = 0; j < 3; j++) { 
      for (int bit = 7; bit >= 0; bit--) {
        // LED Neopixel is active-low
        if ((rgb_data_seq[j] >> bit) & 0x1) {
            sequence_data[pwm_index] = ZERO_HIGH;
        } else {
            sequence_data[pwm_index] = ONE_HIGH;
        }
        pwm_index++;
      }
    }
  }

  pwm_sequence.length = 16*24;
  nrfx_pwm_simple_playback(&PWM_INST, &pwm_sequence, 1, NRFX_PWM_FLAG_STOP);
  
  while (!nrf_pwm_event_check(PWM_INST.p_registers, NRF_PWM_EVENT_STOPPED));
  nrf_pwm_event_clear(PWM_INST.p_registers, NRF_PWM_EVENT_STOPPED);

  nrf_delay_us(RESET_DELAY_US);
  //printf("In show: \n");
  //print_sequence_data();
}

void neopixel_set_color(uint8_t led, color_name_t color) {
  color_t rgb_value = COLOR_TABLE[color];
  neopixel_set_rgb(led, rgb_value.r, rgb_value.g, rgb_value.b);
  neopixel_show();
}

void neopixel_set_color_all(color_name_t color) {
  for (uint8_t i = 0; i < 16; i++) {
    neopixel_set_color(i, color);
  }
  neopixel_show();
}

void neopixel_clear(uint8_t led) {
  neopixel_set_color(led, COLOR_BLACK);
  neopixel_show();
}

void neopixel_clear_all(void) {
  for (uint8_t i = 0; i < 16; i++) {
    neopixel_set_color(i, COLOR_BLACK);
  }
  neopixel_show();
}