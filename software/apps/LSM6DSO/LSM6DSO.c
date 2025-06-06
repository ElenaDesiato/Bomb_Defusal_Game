/* Driver for the accelerometer/gyroscope features of the LSM6DSO
    * use i2c to communicate with it 
    * use timer to make it non-blocking: periodically use timer callbacks to set global variable to most recent measurement 
      & have a non-blocking function return newest measurement when needed
    * Reference 1: Lab6 I2c sensors
    * Reference 2: Arduino library https://github.com/sparkfun/SparkFun_Qwiic_6DoF_LSM6DSO_Arduino_Library
*/

#include <stdbool.h>
#include <stdint.h>
#include <math.h>

#include "LSM6DSO.h"
#include "nrf_delay.h"
#include "app_timer.h"


#define CONCAT(msbits, lsbits) ((int16_t)((msbits << 8) | lsbits))

#define ACC_MEASUREMENT_INTERVAL 100    // interval between measurements in ms

APP_TIMER_DEF(measurement_timer); 

// Pointer to an initialized I2C instance to use for transactions
static const nrf_twi_mngr_t* i2c_manager = NULL;

//Global variables for measurements
static lsm6dso_measurement_t curr_measurement = {0}; 
static float curr_tilt = 0.0; 
static float curr_pitch = 0.0; 
static float curr_roll = 0.0; 
static bool isReady = false; 


// 4 helper functions copied from lab 6
/*
Helper function: read length bytes from I2c device
   i2c_addr - address of device to read from 
   buf - pointer to where data should be read to
   length - length of data to be read (in Bytes)
static void i2c_read_bytes(uint8_t i2c_addr, uint8_t* buf, uint8_t length) {
  nrf_twi_mngr_transfer_t const read_transfer[] = {
    NRF_TWI_MNGR_READ(i2c_addr, buf, length, 0)
  };
  ret_code_t result = nrf_twi_mngr_perform(i2c_manager, NULL, read_transfer, 1, NULL);
  if (result != NRF_SUCCESS) {
    printf("I2C transaction failed! Error: %lX\n", result);
  }
}
  
Helper function: send length number of data bytes to the given i2c address
    i2c_addr - address of device to write to 
    data - pointer to data to be written
    length - length of data to be written
static void i2c_write_bytes(uint8_t i2c_addr, uint8_t* data, uint8_t length) {
    nrf_twi_mngr_transfer_t const write_transfer[] = {
        NRF_TWI_MNGR_WRITE(i2c_addr, data, length, 0),
    }; 
    ret_code_t result = nrf_twi_mngr_perform(i2c_manager, NULL, write_transfer, 1, NULL);
    if (result != NRF_SUCCESS) {
        printf("I2C transaction failed! Error: %lX\n", result);
    }
}
*/

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
static float acceleration_to_angle(lsm6dso_measurement_t* m){
  // multiply by sensitivity, add bias, convert mg -> g
  float x_val = (m->x_axis * 0.061 + 20)/1000.0;  
  float y_val = (m->y_axis * 0.061 + 20)/1000.0;  
  float z_val = (m->z_axis * 0.061 + 20)/1000.0;  
  // use formula to compute tilt angle
  return atan(sqrt(x_val*x_val + y_val*y_val) / z_val) * 180.0/M_PI;
}

static float acceleration_to_pitch(lsm6dso_measurement_t* m){
  // multiply by sensitivity, add bias, convert mg -> g
  float x_val = (m->x_axis * 0.061 + 20)/1000.0;  
  float y_val = (m->y_axis * 0.061 + 20)/1000.0;  
  float z_val = (m->z_axis * 0.061 + 20)/1000.0;  
  // use formula to compute tilt angle
  return atan(x_val / sqrt(z_val*z_val + y_val*y_val)) * 180.0/M_PI;
}

static float acceleration_to_roll(lsm6dso_measurement_t* m){
  // multiply by sensitivity, add bias, convert mg -> g
  float x_val = (m->x_axis * 0.061 + 20)/1000.0;  
  float y_val = (m->y_axis * 0.061 + 20)/1000.0;  
  float z_val = (m->z_axis * 0.061 + 20)/1000.0;  
  // use formula to compute tilt angle
  return atan(y_val / sqrt(z_val*z_val + x_val*x_val)) * 180.0/M_PI;
}

// Helper function to read raw accelerometer data, inspired by lab6
static lsm6dso_measurement_t get_raw_accel_data(void) {
  isReady = false; 
  lsm6dso_measurement_t measurement = {0};
  uint8_t x_lsbits = i2c_read_reg(LSM6DSO_DEF_ADDR, OUTX_L_A); 
  uint8_t x_msbits = i2c_read_reg(LSM6DSO_DEF_ADDR, OUTX_H_A); 
  measurement.x_axis = (float)CONCAT(x_msbits, x_lsbits);
  uint8_t y_lsbits = i2c_read_reg(LSM6DSO_DEF_ADDR, OUTY_L_A); 
  uint8_t y_msbits = i2c_read_reg(LSM6DSO_DEF_ADDR, OUTY_H_A); 
  measurement.y_axis = (float)CONCAT(y_msbits, y_lsbits);
  uint8_t z_lsbits = i2c_read_reg(LSM6DSO_DEF_ADDR, OUTZ_L_A); 
  uint8_t z_msbits = i2c_read_reg(LSM6DSO_DEF_ADDR, OUTZ_H_A); 
  measurement.z_axis = (float)CONCAT(z_msbits, z_lsbits);
  return measurement;
}

// Function that sets measurement periodically
static void set_measurement(void* p_context) {
  curr_measurement = get_raw_accel_data(); 
  curr_tilt = acceleration_to_angle(&curr_measurement); 
  curr_pitch = acceleration_to_pitch(&curr_measurement);
  curr_roll = acceleration_to_roll(&curr_measurement);
  isReady = true; 
}

// Return true iff measurement is ready - nonblocking
bool lsm6dso_is_ready(void) {
  return isReady; 
}

// Return tilt angle in degrees - nonblocking
float lsm6dso_get_tilt(void) {
  return curr_tilt; 
}

float lsm6dso_get_pitch(void) {
  return curr_pitch; 
}

float lsm6dso_get_roll(void) {
  return curr_roll; 
}

void lsm6dso_stop(void){
  app_timer_stop(measurement_timer);
}

void lsm6dso_start(void){
  app_timer_start(measurement_timer, APP_TIMER_TICKS(ACC_MEASUREMENT_INTERVAL), NULL);
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

    isReady = false; 
    curr_tilt = 0.0;

    app_timer_init();
    app_timer_create(&measurement_timer, APP_TIMER_MODE_REPEATED, set_measurement);
    // APP_TIMER_TICKS converts ms to timer ticks
    //app_timer_start(measurement_timer, APP_TIMER_TICKS(ACC_MEASUREMENT_INTERVAL), NULL); 
}

