#ifndef __LSM6DSOIMU_H__
#define __LSM6DSOIMU_H__

#include <stdint.h>
#include "nrf_twi_mngr.h"

#define LSM6DSO_DEF_ADDR 0x6B
#define LSM6DSO_ALT_ADDR 0x6A

// Measurement data type
typedef struct {
  float x_axis;
  float y_axis;
  float z_axis;
} lsm6dso_measurement_t;

// copied from arduino library
typedef enum {

  FUNC_CFG_ACCESS        = 0x01,  
  LSM6DO_PIN_CTRL        = 0x02, 

  FIFO_CTRL1             = 0x07,
  FIFO_CTRL2             = 0x08,
  FIFO_CTRL3             = 0x09,
  FIFO_CTRL4             = 0x0A,

  COUNTER_BDR_REG1       = 0x0B,   
  COUNTER_BDR_REG2       = 0x0C,  

  INT1_CTRL              = 0x0D,
  INT2_CTRL              = 0x0E,
  WHO_AM_I_REG           = 0x0F,
  CTRL1_XL               = 0x10,
  CTRL2_G                = 0x11,
  CTRL3_C                = 0x12,
  CTRL4_C                = 0x13,
  CTRL5_C                = 0x14,
  CTRL6_C                = 0x15,
  CTRL7_G                = 0x16,
  CTRL8_XL               = 0x17,
  CTRL9_XL               = 0x18,
  CTRL10_C               = 0x19,
  ALL_INT_SRC            = 0x1A,
  WAKE_UP_SRC            = 0x1B,
  TAP_SRC                = 0x1C,
  D6D_SRC                = 0x1D,
  STATUS_REG             = 0x1E,
  OUT_TEMP_L             = 0x20,
  OUT_TEMP_H             = 0x21,
  OUTX_L_G               = 0x22,
  OUTX_H_G               = 0x23,
  OUTY_L_G               = 0x24,
  OUTY_H_G               = 0x25,
  OUTZ_L_G               = 0x26,
  OUTZ_H_G               = 0x27,

  OUTX_L_A               = 0x28,
  OUTX_H_A               = 0x29,
  OUTY_L_A               = 0x2A,
  OUTY_H_A               = 0x2B,
  OUTZ_L_A               = 0x2C,
  OUTZ_H_A               = 0x2D,

  EMB_FUNC_STATUS_MP     = 0x35,
  FSM_FUNC_STATUS_A_MP   = 0x36,
  FSM_FUNC_STATUS_B_MP   = 0x37,
  STATUS_MASTER_MAINPAGE = 0x39,

  FIFO_STATUS1           = 0x3A,
  FIFO_STATUS2           = 0x3B,

  TIMESTAMP0_REG         = 0x40,
  TIMESTAMP1_REG         = 0x41,
  TIMESTAMP2_REG         = 0x42,
  TIMESTAMP3_REG         = 0x43,

  TAP_CFG0               = 0x56,  
  TAP_CFG1               = 0x57,   
  TAP_CFG2               = 0x58, 
  TAP_THS_6D             = 0x59,
  INT_DUR2               = 0x5A,
  WAKE_UP_THS            = 0x5B,
  WAKE_UP_DUR            = 0x5C,
  FREE_FALL              = 0x5D,
  MD1_CFG                = 0x5E,
  MD2_CFG                = 0x5F,

  I3C_BUS_AVB            = 0x62,   
  INTERNAL_FREQ_FINE     = 0x63,  

  INT_OIS                = 0x6F,  
  CTRL1_OIS              = 0x70,  
  CTRL2_OIS              = 0x71,  
  CTRL3_OIS              = 0x72,  
  X_OFS_USR              = 0x73,  
  Y_OFS_USR              = 0x74,  
  Z_OFS_USR              = 0x75,  

  FIFO_DATA_OUT_TAG      = 0x78,  
  FIFO_DATA_OUT_X_L      = 0x79,  
  FIFO_DATA_OUT_X_H      = 0x7A,  
  FIFO_DATA_OUT_Y_L      = 0x7B,  
  FIFO_DATA_OUT_Y_H      = 0x7C,  
  FIFO_DATA_OUT_Z_L      = 0x7D,  
  FIFO_DATA_OUT_Z_H      = 0x7E,  
} lsm6dso_reg_t; 

void lsm6dso_init(const nrf_twi_mngr_t* i2c);

// Returns true iff measurement is ready - nonblocking
bool lsm6dso_is_ready(void); 

// Returns tilt angle in degrees - non-blocking
// Should NOT be called unless lsm6dso_is_ready returns true
float lsm6dso_get_tilt(void); 
float lsm6dso_get_pitch(void); 
float lsm6dso_get_row(void); 


#endif