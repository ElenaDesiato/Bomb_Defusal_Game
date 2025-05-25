#pragma once

#include "nrf_twi_mngr.h"

#define SX1509_ADDR 0x70

bool sx1509_init(const nrf_twi_mngr_t* twi);
bool sx1509_is_connected(const nrf_twi_mngr_t* twi);
void sx1509_pin_config_input_pullup(uint8_t pin);
void sx1509_pin_config_output(uint8_t pin);

bool sx1509_pin_read(uint8_t pin);
void sx1509_pin_write(uint8_t pin, bool value);
void sx1509_pin_clear(uint8_t pin);
void sx1509_pin_set(uint8_t pin);

void scan_i2c_bus(const nrf_twi_mngr_t* mngr);