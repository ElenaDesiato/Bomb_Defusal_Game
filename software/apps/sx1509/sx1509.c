// SX1509 driver for Microbit_v2
// Code inspiration: i2c lab

#include <stdbool.h>
#include <stdint.h>

#include "nrf_delay.h"
#include "sx1509.h"
#include "app_util_platform.h" 

#define REG_RESET     0x7D
#define REG_DIR_A     0x0F
#define REG_PULLUP_A  0x07
#define REG_DATA_A    0x11
#define REG_DIR_B     0x0E
#define REG_PULLUP_B  0x06
#define REG_DATA_B    0x10

#define BANK(pin) (pin <= 7 ? 0 : 1)
#define GET_X_PIN(x) (1 << (x - (BANK(x)*8)))

static const nrf_twi_mngr_t* twi_mngr = NULL;

static bool write_reg(uint8_t reg, uint8_t val) {
  uint8_t data_to_write[2] = { reg, val };
  nrf_twi_mngr_transfer_t transfer = NRF_TWI_MNGR_WRITE(SX1509_ADDR, data_to_write, 2, 0);
  ret_code_t ret_code = nrf_twi_mngr_perform(twi_mngr, NULL, &transfer, 1, NULL);
  if (ret_code != NRF_SUCCESS) {
    printf("Error: %ld\n",ret_code);
    return false;
  }
  return true; 
}

static uint8_t read_reg(uint8_t reg) {
  uint8_t res = 0;
  nrf_twi_mngr_transfer_t transfers[] = {
    NRF_TWI_MNGR_WRITE(SX1509_ADDR, &reg, 1, NRF_TWI_MNGR_NO_STOP),
    NRF_TWI_MNGR_READ(SX1509_ADDR, &res, 1, 0)
  };
  ret_code_t ret_code = nrf_twi_mngr_perform(twi_mngr, NULL, transfers, 2, NULL);
  if (ret_code != NRF_SUCCESS) {
    printf("Error: %ld\n",ret_code);
    return 0;
  }
  return res;
}

bool sx1509_is_connected(const nrf_twi_mngr_t* i2c) {
  uint8_t dummy;
  nrf_twi_mngr_transfer_t transfer = NRF_TWI_MNGR_READ(SX1509_ADDR, &dummy, 1, 0);
  ret_code_t ret_code = nrf_twi_mngr_perform(i2c, NULL, &transfer, 1, NULL);
  if (ret_code != NRF_SUCCESS) {
    printf("Error: %ld\n", ret_code);
    return false;
  }
  return true;
}

bool sx1509_init(const nrf_twi_mngr_t* twi) {
  twi_mngr = twi;
  write_reg(REG_RESET, 0x12);
  write_reg(REG_RESET, 0x34);
  nrf_delay_ms(10);
  return true;
}

// Helper function: Read register & set pin's bit to 1
static void set_pin_bit(uint8_t reg, uint8_t pin) {
  uint8_t new_val = read_reg(reg);
  new_val |= GET_X_PIN(pin);
  write_reg(reg, new_val);
}

// Helper function: Read register & set pin's bit to 0
static void clear_pin_bit(uint8_t reg, uint8_t pin) {
  uint8_t new_val = read_reg(reg);
  new_val &= (~GET_X_PIN(pin));
  write_reg(reg, new_val);
}

void sx1509_pin_config_input_pullup(uint8_t pin) {

  uint8_t reg_dir = BANK(pin) ? REG_DIR_B: REG_DIR_A; 
  uint8_t reg_pullup = BANK(pin) ? REG_PULLUP_B : REG_PULLUP_A; 

  set_pin_bit(reg_dir,pin); 
  set_pin_bit(reg_pullup, pin); 
}

void sx1509_pin_config_output(uint8_t pin) {
  uint8_t reg_dir = BANK(pin) ? REG_DIR_B : REG_DIR_A; 
  clear_pin_bit(reg_dir,pin); 
}

bool sx1509_pin_read(uint8_t pin) {
  uint8_t reg_data = BANK(pin) ? REG_DATA_B : REG_DATA_A; 
  uint8_t val = read_reg(reg_data);
  return (val & (GET_X_PIN(pin))) != 0;
}

void sx1509_pin_clear(uint8_t pin) {
  uint8_t reg_data = BANK(pin) ? REG_DATA_B : REG_DATA_A;
  clear_pin_bit(reg_data,pin); 
}

void sx1509_pin_set(uint8_t pin){
  uint8_t reg_data = BANK(pin) ? REG_DATA_B : REG_DATA_A;
  set_pin_bit(reg_data,pin); 
}

void sx1509_pin_write(uint8_t pin, bool value){
  if (value) {
    sx1509_pin_set(pin);
  } else {
    sx1509_pin_clear(pin);
  }
}

// Helper function for debugging purposes that scans for I2C device addresses
void scan_i2c_bus(const nrf_twi_mngr_t* mngr) {
  printf("Scanning I2C bus...\n");

  uint8_t addr = 0;
  while(true){
    uint8_t dummy;
    nrf_twi_mngr_transfer_t transfer = NRF_TWI_MNGR_READ(addr, &dummy, 1, 0);
    ret_code_t error = nrf_twi_mngr_perform(mngr, NULL, &transfer, 1, NULL);
    if (error == NRF_SUCCESS) {
      printf("-> Found I2C device at 0x%x\n", addr);
    }
    if (addr == 0xFF) break;
    addr++; 
  }
}