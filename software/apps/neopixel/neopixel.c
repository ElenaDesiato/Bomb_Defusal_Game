#include <stdint.h>
#include <stdbool.h>
#include "nrfx_pwm.h"
#include "nrf_delay.h"
#include "nrf_gpio.h" 
#include "neopixel.h" 
#include "microbit_v2.h"


#define NEOPIXEL_PWM_TOP_VALUE 20
#define ONE_HIGH         13  // PWM value for '1' bit high duration
#define ZERO_HIGH        6   // PWM value for '0' bit high duration
#define RESET_DELAY_US   280 // Standard WS2812B reset time is >50us, Adafruit recommends >280us for reliability

#define MAX_LED_PER_DEV  16  // Max LED supported per device (can be adjusted if needed)
#define BITS_PER_LED     24

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

// This table is now used by neopixel_set_color
// Values are for GRB order (typical for WS2812B)
const color_t COLOR_TABLE[COLOR_COUNT] = {
  [COLOR_BLACK]   = {0, 0, 0},
  [COLOR_WHITE]   = {16, 16, 16}, // Reduced brightness
  [COLOR_RED]     = {16, 0, 0},   // R, G, B
  [COLOR_GREEN]   = {0, 16, 0},
  [COLOR_BLUE]    = {0, 0, 16},
  [COLOR_YELLOW]  = {16, 16, 0},
  [COLOR_CYAN]    = {0, 16, 16},
  [COLOR_MAGENTA] = {16, 0, 16},
};

static color_t rgb_data[NEO_DEV_COUNT][MAX_LED_PER_DEV];
static nrf_pwm_values_individual_t sequence_data[NEO_DEV_COUNT][MAX_LED_PER_DEV * BITS_PER_LED];
static nrf_pwm_sequence_t pwm_sequences[NEO_DEV_COUNT];


static const nrfx_pwm_t PWM_INST = NRFX_PWM_INSTANCE(0);

void neopixel_init(void) {
  nrfx_pwm_config_t config = {
    .output_pins = {
      NEOPIXEL_PINS[NEO_RING],  // Mapped to PWM channel 0
      NEOPIXEL_PINS[NEO_JEWEL], // Mapped to PWM channel 1
      NEOPIXEL_PINS[NEO_STICK], // Mapped to PWM channel 2
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

  for (int device_idx = 0; device_idx < NEO_DEV_COUNT; device_idx++) {
    pwm_sequences[device_idx].values.p_individual = sequence_data[device_idx];
    pwm_sequences[device_idx].length              = (uint16_t)(LED_COUNT[device_idx] * BITS_PER_LED);
    pwm_sequences[device_idx].repeats             = 0;
    pwm_sequences[device_idx].end_delay           = 0;
  }
}

// === Pixel Logic === //

void neopixel_set_rgb(neopixel_device_t device, uint8_t index, uint8_t r, uint8_t g, uint8_t b) {
  if (device < NEO_DEV_COUNT && index < LED_COUNT[device]) {
    rgb_data[device][index].r = r;
    rgb_data[device][index].g = g;
    rgb_data[device][index].b = b;
  }
}

// Prepares the PWM sequence data for a specific Neopixel device
static void prepare_sequence_data(neopixel_device_t device) {
  uint16_t current_pwm_idx = 0;
  for (uint8_t led_idx = 0; led_idx < LED_COUNT[device]; led_idx++) {
    uint8_t g_val = rgb_data[device][led_idx].g;
    uint8_t r_val = rgb_data[device][led_idx].r;
    uint8_t b_val = rgb_data[device][led_idx].b;

    uint32_t grb_color = ((uint32_t)g_val << 16) | ((uint32_t)r_val << 8) | (uint32_t)b_val;

    // Iterate through each of the 24 bits (MSB first)
    for (int bit_pos = BITS_PER_LED - 1; bit_pos >= 0; bit_pos--) {
      // Determine if the current bit is a '1' or '0'
      uint16_t pwm_val_for_bit = ((grb_color >> bit_pos) & 0x01) ? ZERO_HIGH : ONE_HIGH;

      sequence_data[device][current_pwm_idx].channel_0 = 0;
      sequence_data[device][current_pwm_idx].channel_1 = 0;
      sequence_data[device][current_pwm_idx].channel_2 = 0;
      sequence_data[device][current_pwm_idx].channel_3 = 0; // Not used, but good practice to init

      switch (device) {
        case NEO_RING:  // NEO_RING is connected to output_pins[0] (PWM Channel 0)
          sequence_data[device][current_pwm_idx].channel_0 = pwm_val_for_bit;
          break;
        case NEO_JEWEL: // NEO_JEWEL is connected to output_pins[1] (PWM Channel 1)
          sequence_data[device][current_pwm_idx].channel_1 = pwm_val_for_bit;
          break;
        case NEO_STICK: // NEO_STICK is connected to output_pins[2] (PWM Channel 2)
          sequence_data[device][current_pwm_idx].channel_2 = pwm_val_for_bit;
          break;
      }
      current_pwm_idx++;
    }
  }
}

void neopixel_show(neopixel_device_t device) {
  prepare_sequence_data(device);

  nrfx_err_t err = nrfx_pwm_simple_playback(&PWM_INST, &pwm_sequences[device], 1, NRFX_PWM_FLAG_STOP);
  
  if (err == NRFX_SUCCESS) {
    // Wait for the PWM sequence to complete by polling the STOPPED event
    while (!nrf_pwm_event_check(PWM_INST.p_registers, NRF_PWM_EVENT_STOPPED)) {
        // Could add __WFE(); for power saving if interrupts are configured for other things
    }
    nrf_pwm_event_clear(PWM_INST.p_registers, NRF_PWM_EVENT_STOPPED);
  } else {
    // Handle error (e.g., NRF_LOG_ERROR("PWM playback failed: %d", err); if logging is set up)
  }

  nrf_delay_us(RESET_DELAY_US);
}

void neopixel_set_color(neopixel_device_t device, uint8_t led_index, color_name_t color_name) {
  if (device < NEO_DEV_COUNT && led_index < LED_COUNT[device] && color_name < COLOR_COUNT) {
    // Look up RGB values from the COLOR_TABLE using the color_name enum
    color_t c = COLOR_TABLE[color_name]; // This uses the const COLOR_TABLE defined in this .c file
    neopixel_set_rgb(device, led_index, c.r, c.g, c.b);
  }
}

void neopixel_set_color_all(neopixel_device_t device, color_name_t color_name) {
  if (device < NEO_DEV_COUNT && color_name < COLOR_COUNT) {
    for (int i = 0; i < LED_COUNT[device]; i++) {
      neopixel_set_color(device, i, color_name);
    }
  }
}

void neopixel_clear(neopixel_device_t device, uint8_t led_index) {
   if (device < NEO_DEV_COUNT && led_index < LED_COUNT[device]) {
     neopixel_set_color(device, led_index, COLOR_BLACK);
   }
}

void neopixel_clear_all(neopixel_device_t device) {
  if (device < NEO_DEV_COUNT) {
    neopixel_set_color_all(device, COLOR_BLACK);
  }
}