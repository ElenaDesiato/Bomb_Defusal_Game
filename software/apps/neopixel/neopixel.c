/* Generalized Neopixel Driver for Microbit_v2
Goal: Drive multiple neopixel items (Ring, Jewel, Strip) via 1 pwm instance (instance 0)
  - have to use "individual" mode instead of "common"
  - Jewel also has RGBW format instead of RGB which is an extra byte
Basic code structure reference: Lab5 pwm_square_tone
*/

#include <stdint.h>
#include <stdbool.h>
#include "nrfx_pwm.h"
#include "nrf_delay.h"
#include "nrf_gpio.h" 
#include "neopixel.h" 
#include "microbit_v2.h"
#include <stdio.h>


#define NEOPIXEL_PWM_TOP_VALUE 20
#define ONE_HIGH         13 
#define ZERO_HIGH        6   
#define RESET_DELAY_US   280 // Standard WS2812B reset time is >50us, Adafruit recommends >280us for reliability

#define MAX_BITS 16*24  //the neopixel sends the most bits
#define MAX_LED_PER_DEV  16  
#define BITS_PER_LED(device)  (((device) == NEO_JEWEL) ? 32 : 24)
#define BITS_PER_DEVICE(device) (BITS_PER_LED(device) * LED_COUNT[device])

static const uint8_t LED_COUNT[NEO_DEV_COUNT] = {
  [NEO_RING]  = 16,
  [NEO_JEWEL] = 7,
  [NEO_STICK] = 8,
};

// These map to PWM channels 0, 1, 2 respectively due to .output_pins order in nrfx_pwm_config_t
static const uint32_t NEOPIXEL_PINS[NEO_DEV_COUNT] = {
  [NEO_RING]  = EDGE_P9,
  [NEO_JEWEL] = EDGE_P2,
  [NEO_STICK] = EDGE_P1,
};

const color_t COLOR_TABLE[COLOR_COUNT] = {
  [COLOR_BLACK]   = {0, 0, 0,0},
  [COLOR_WHITE]   = {16, 16, 16,0},
  [COLOR_RED]     = {16, 0, 0,0},  
  [COLOR_GREEN]   = {0, 16, 0,0},
  [COLOR_BLUE]    = {0, 0, 16,0},
  [COLOR_YELLOW]  = {16, 16, 0,0},
  [COLOR_CYAN]    = {0, 16, 16,0},
  [COLOR_MAGENTA] = {16, 0, 16,0},
};

// PWM configuration
static const nrfx_pwm_t PWM_INST = NRFX_PWM_INSTANCE(0);

// contains one rgbw value for every led for every device
static color_t rgb_data[NEO_DEV_COUNT][MAX_LED_PER_DEV];

// individual sequence that contains duty cycle values for all LEDs
// each entry has one value for each channel
static nrf_pwm_values_individual_t combined_sequence[MAX_BITS];

// Sequence structure for configuring DMA
static nrf_pwm_sequence_t pwm_sequence = {
  .values.p_individual = combined_sequence,
  .length = 0,
  .repeats = 0,
  .end_delay = 0,
};

void print_combined_sequence(int length); 

void neopixel_init(void) {
  nrfx_pwm_config_t config = {
    .output_pins = {
      NEOPIXEL_PINS[NEO_RING],  
      NEOPIXEL_PINS[NEO_JEWEL], 
      NEOPIXEL_PINS[NEO_STICK], 
      NRFX_PWM_PIN_NOT_USED
    },
    .irq_priority = NRFX_PWM_DEFAULT_CONFIG_IRQ_PRIORITY,
    .base_clock   = NRF_PWM_CLK_16MHz,
    .count_mode   = NRF_PWM_MODE_UP,
    .top_value    = NEOPIXEL_PWM_TOP_VALUE,
    .load_mode    = NRF_PWM_LOAD_INDIVIDUAL,
    .step_mode    = NRF_PWM_STEP_AUTO
  };
  nrfx_pwm_init(&PWM_INST, &config, NULL); 
}

void neopixel_set_rgbw(neopixel_device_t device, uint8_t index, uint8_t r, uint8_t g, uint8_t b, uint8_t w) {
  if (device < NEO_DEV_COUNT && index < LED_COUNT[device]) {
    rgb_data[device][index].r = r;
    rgb_data[device][index].g = g;
    rgb_data[device][index].b = b;
    rgb_data[device][index].w = w;
    //printf("[neopixel_set_rgb] device=%d, index=%d, r=%d, g=%d, b=%d\n", device, index, r, g, b);
  }
}

// Helper function: prepare sequence data for all channels combined (as required in individual mode)
static void prepare_sequence_data(neopixel_device_t device) {
  int led_count = LED_COUNT[device];
  uint16_t pwm_idx = 0;

  for (int led = 0; led < led_count; led++) {
      uint8_t rgbw_data_seq[4] = { rgb_data[device][led].g, rgb_data[device][led].r, rgb_data[device][led].b , rgb_data[device][led].w};
      for (int c = 0; c < 4; c++) {

        // Skip white value if neopixel device is not jewel
        if (c == 3 && device != NEO_JEWEL) break; 

        for (int bit = 7; bit >= 0; bit--) {
          // Clear values for all channels to avoid ghost colors
          combined_sequence[pwm_idx].channel_0 = 0;
          combined_sequence[pwm_idx].channel_1 = 0;
          combined_sequence[pwm_idx].channel_2 = 0;
          combined_sequence[pwm_idx].channel_3 = 0;
          uint16_t pwm_val = ((rgbw_data_seq[c]>> bit) & 0x1) ? ZERO_HIGH : ONE_HIGH;
          switch (device) {
            case NEO_RING:  
              combined_sequence[pwm_idx].channel_0 = pwm_val; 
              break;
            case NEO_JEWEL: 
              combined_sequence[pwm_idx].channel_1 = pwm_val; 
              break;
            case NEO_STICK: 
              combined_sequence[pwm_idx].channel_2 = pwm_val; 
              break;
          }
          pwm_idx++;
        }
      }
  }
  // Clear rest of buffer bc otherwise ghost colors appear
  for (; pwm_idx < MAX_BITS; pwm_idx++) {
    combined_sequence[pwm_idx].channel_0 = 0;
    combined_sequence[pwm_idx].channel_1 = 0;
    combined_sequence[pwm_idx].channel_2 = 0;
    combined_sequence[pwm_idx].channel_3 = 0;
  }
}

void neopixel_show(neopixel_device_t device) {
  prepare_sequence_data(device);
  pwm_sequence.length = BITS_PER_DEVICE(device) * 4;
  nrfx_pwm_simple_playback(&PWM_INST, &pwm_sequence, 1, NRFX_PWM_FLAG_STOP);
  
  while (!nrf_pwm_event_check(PWM_INST.p_registers, NRF_PWM_EVENT_STOPPED));
  nrf_pwm_event_clear(PWM_INST.p_registers, NRF_PWM_EVENT_STOPPED);

  nrf_delay_us(RESET_DELAY_US);
}

void neopixel_set_color(neopixel_device_t device, uint8_t led_index, color_name_t color_name) {
  if (device < NEO_DEV_COUNT && led_index < LED_COUNT[device] && color_name < COLOR_COUNT) {
    color_t c = COLOR_TABLE[color_name];
    neopixel_set_rgbw(device, led_index, c.r, c.g, c.b,c.w);
  }
  //printf("[neopixel_set_color] device=%d, led_index=%d, color_name=%d\n", device, led_index, color_name);
  neopixel_show(device);
}

void neopixel_set_color_all(neopixel_device_t device, color_name_t color_name) {
  if (device < NEO_DEV_COUNT && color_name < COLOR_COUNT) {
    int led_count = LED_COUNT[device];
    for (int i = 0; i < led_count; i++) {
      neopixel_set_color(device, i, color_name);
    }
  }
  neopixel_show(device);
}

void neopixel_clear(neopixel_device_t device, uint8_t led_index) {
   if (device < NEO_DEV_COUNT && led_index < LED_COUNT[device]) {
     neopixel_set_color(device, led_index, COLOR_BLACK);
   }
  neopixel_show(device);
}

void neopixel_clear_all(neopixel_device_t device) {
  if (device < NEO_DEV_COUNT) {
    neopixel_set_color_all(device, COLOR_BLACK);
  }
  neopixel_show(device);
}

// Helper function to print the contents of the combined_sequence buffer
void print_combined_sequence(int length) {
    printf("Combined sequence contents (length=%d):\n", length);
    for (int i = 0; i < length; i++) {
        printf("[%3d] ch0=%2d ch1=%2d ch2=%2d ch3=%2d\n",
            i,
            combined_sequence[i].channel_0,
            combined_sequence[i].channel_1,
            combined_sequence[i].channel_2,
            combined_sequence[i].channel_3
        );
    }
}