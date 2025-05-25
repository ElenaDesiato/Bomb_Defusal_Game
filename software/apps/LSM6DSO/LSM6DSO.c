/* Driver for the accelerometer/gyroscope features of the LSM6DSO
    * use i2c to communicate with it 
    * Reference 1: Lab6 I2c sensors
    * Reference 2: Arduino library https://github.com/sparkfun/SparkFun_Qwiic_6DoF_LSM6DSO_Arduino_Library
    * Reference 3: temp_driver (for structure of nonblocking driver)
*/

#include <stdbool.h>
#include <stdint.h>

#include "LSM6DSO.h"
#include "nrf_delay.h"

#define CONCAT(msbits, lsbits) ((int16_t)((msbits << 8) | lsbits))

// Pointer to an initialized I2C instance to use for transactions
static const nrf_twi_mngr_t* i2c_manager = NULL;

// Global variables for callback function
static void (*callback_fn)(float, void*) = NULL;
static void* callback_context = NULL;

// 4 helper functions copied from lab 6
/* 
Helper function: read length bytes from I2c device
   i2c_addr - address of device to read from 
   buf - pointer to where data should be reda to
   length - length of data to be read (in Bytes)
*/
static void i2c_read_bytes(uint8_t i2c_addr, uint8_t* buf, uint8_t length) {
  nrf_twi_mngr_transfer_t const read_transfer[] = {
    NRF_TWI_MNGR_READ(i2c_addr, buf, length, 0)
  };
  ret_code_t result = nrf_twi_mngr_perform(i2c_manager, NULL, read_transfer, 1, NULL);
  if (result != NRF_SUCCESS) {
    printf("I2C transaction failed! Error: %lX\n", result);
  }
}

/* Helper function: send length number of data bytes to the given i2c address
    i2c_addr - address of device to write to 
    data - pointer to data to be written
    length - length of data to be written
*/
static void i2c_write_bytes(uint8_t i2c_addr, uint8_t* data, uint8_t length) {
    nrf_twi_mngr_transfer_t const write_transfer[] = {
        NRF_TWI_MNGR_WRITE(i2c_addr, data, length, 0),
    }; 
    ret_code_t result = nrf_twi_mngr_perform(i2c_manager, NULL, write_transfer, 1, NULL);
    if (result != NRF_SUCCESS) {
        printf("I2C transaction failed! Error: %lX\n", result);
    }
}

/* Helper function: perform a 1-byte I2C read of a given register
    i2c_addr - address of the device to read from 
    reg_addr - address of the register within the device to read
*/
static uint8_t i2c_read_reg(uint8_t i2c_addr, uint8_t reg_addr) {
  uint8_t rx_buf = 0;
  nrf_twi_mngr_transfer_t const read_transfer[] = {
    NRF_TWI_MNGR_WRITE(i2c_addr, &reg_addr, 1, NRF_TWI_MNGR_NO_STOP),
    NRF_TWI_MNGR_READ(i2c_addr, &rx_buf, 1, 0)
  };
  ret_code_t result = nrf_twi_mngr_perform(i2c_manager, NULL, read_transfer, 2, NULL);
  if (result != NRF_SUCCESS) {
    printf("I2C transaction failed! Error: %lX\n", result);
  }
  return rx_buf;
}

/* Helper function to perform a 1-byte I2C write of a given register
    2c_addr - address of the device to write to
    reg_addr - address of the register within the device to write
    data - data to write to register
*/
static void i2c_write_reg(uint8_t i2c_addr, uint8_t reg_addr, uint8_t data) {
  uint8_t data_to_write[] = {reg_addr, data};
  nrf_twi_mngr_transfer_t const write_transfer[] = {
    NRF_TWI_MNGR_WRITE(i2c_addr, data_to_write, 2, 0),
  };
  ret_code_t result = nrf_twi_mngr_perform(i2c_manager, NULL, write_transfer, 1, NULL);
  if (result != NRF_SUCCESS) {
    printf("I2C transaction failed! Error: %lX\n", result);
  }
}

// Helper function to convert accelerometer reading to angle, almost from lab6
static void acceleration_to_angle(lsm6dso_measurement_t* m){
  // multiply by sensitivity, add bias, convert mg -> g
  float x_val = (m->x_axis * 0.061 + 20)/1000;  
  float y_val = (m->y_axis * 0.061 + 20)/1000;  
  float z_val = (m->z_axis * 0.061 + 20)/1000;  
  // use formulas to compute angles
  float x_denom= sqrt(pow(y_val,2) + pow(z_val,2));
  float y_denom = sqrt(pow(x_val,2) + pow(z_val,2));
  m->x_axis = (x_denom != 0) ? atan(x_val/x_denom) * 180/M_PI : 0;
  m->y_axis = (y_denom != 0) ? atan(y_val/y_denom) * 180/M_PI : 0;
  m->z_axis = (z_val != 0) ? atan(sqrt(pow(x_val,2) + pow(y_val,2))/z_val) * 180/M_PI : 0;
}

// Helper function to read raw accelerometer data, inspired by lab6
static lsm6dso_measurement_t get_raw_accel_data(void) {
  uint8_t x_lsbits = i2c_reg_read(LSM6DSO_DEF_ADDR, OUTX_L_A); 
  uint8_t x_msbits = i2c_reg_read(LSM6DSO_DEF_ADDR, OUTX_H_A); 
  int16_t x_val = (float)CONCAT(x_msbits, x_lsbits);
  uint8_t y_lsbits = i2c_reg_read(LSM6DSO_DEF_ADDR, OUTY_L_A); 
  uint8_t y_msbits = i2c_reg_read(LSM6DSO_DEF_ADDR, OUTY_H_A); 
  int16_t y_val = (float)CONCAT(y_msbits, y_lsbits);
  uint8_t z_lsbits = i2c_reg_read(LSM6DSO_DEF_ADDR, OUTZ_L_A); 
  uint8_t z_msbits = i2c_reg_read(LSM6DSO_DEF_ADDR, OUTZ_H_A); 
  int16_t z_val = (float)CONCAT(z_msbits, z_lsbits);

  lsm303agr_measurement_t measurement = {0};
  measurement.x_axis = (float)x_val;
  measurement.y_axis = (float)y_val;
  measurement.z_axis = (float)z_val;
  return measurement;
}

// Interrupt handler for Accelerometer
void accelerometer_handler(void) {
  NRF_TEMP->EVENTS_DATARDY = 0;

  // Be sure to only call function if non-null
  if (callback_fn) {
    lsm6dso_measurement_t m = get_raw_accel_data(); 
    acceleration_to_angle(&m); 
    callback_fn(m, callback_context);
  }
}

void lsm6dso_get_tilt_nonblocking(void (*callback)(float, void*), void* context) {
  // Save callback
  callback_fn = callback;
  callback_context = context;

  // TO DO: enable interrupts somehow
  // Enable lowest-priority interrupts
  NRF_TEMP->INTENSET = 1;
  NVIC_EnableIRQ(TEMP_IRQn);
  NVIC_SetPriority(TEMP_IRQn, 7);

  // Start temperature sensor
  NRF_TEMP->TASKS_START = 1;

  return;
}

/* Initialize sensor
    * ignore most features as not needed: no FIFO buffer (default off), no interrupts
*/
void lsm6dso_init(const nrf_twi_mngr_t* i2c) {
    i2c_manager = i2c;

    // ---Initialize Accelerometer---
    /* Using control register
        - Boot: 1 (reboot)
        - Block Data Update: 1 (use)
        - HLActive: 0 (default)
        - Push-pull or open-drain: 0 (default)
        - SPI SM: 0 (default)
        - If_inc: 1 (default)
        - SW_reset: 1 (reset)
        -> 0b11000101 = 0xC5
    */
    // can I even do this all at once? 
    i2c_write_reg(LSM6DSO_DEF_ADDR, CTRL3_C, 0xC5);
    nrf_delay_ms(100);

    /* Configure accelerometer 
        - 104Hz, normal mode       - first bits = 0100 
        - full-scale selection: 2g - next bits: 00
        - rest: default stuff      - 00
    */
    i2c_write_reg(LSM6DSO_DEF_ADDR, CTRL1_XL, 0x40);

    // enables accelerometer interrupt on INT1 pin on sensor
    i2c_write_reg(LSM6DSO_DEF_ADDR, INT1_CTRL , 0x01);
}

