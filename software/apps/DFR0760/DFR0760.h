// DFR0769.h

#pragma once

#include "nrf_twi_mngr.h"
#include <stdbool.h>
#include <stdint.h>

//  Configuration 
#define DFR0760_ADDR                  0x40  // Default I2C Address

//  Commands 
#define CMD_HEADER                    0xFD  // Packet Frame Header
#define START_SYNTHESIS               0x01  // Speech synthesis command (e.g., English with encoding 0x00)
#define STOP_SYNTHESIS                0x02  // Stop current synthesis
#define PAUSE_SYNTHESIS               0x03  // Pause current synthesis
#define RECOVER_SYNTHESIS             0x04  // Resume paused synthesis
#define ENTERSAVEELETRI               0x88  // Enter sleep/power-save mode
#define WAKEUP                        0xFF  // Wake up from sleep mode
#define INQUIRYSTATUS                 0x21  // Command to check chip status

//  ACK/Status Values 
#define ACK_SYNTHESIS_ACTIVE_OR_START 0x41  // 'A' - Module is synthesizing or has acknowledged synthesis command
#define ACK_DEVICE_READY              0x4F  // 'O' - Module is ready/idle (OK)
#define ACK_ERROR_CHIP                0x45  // 'E' - Error state
#define ACK_ERROR_BUSY_SYNTHESIS      0x42  // 'B' - Chip busy with another synthesis task

// Initialize DFR0760 module. Only call once at boot.
bool DFR0760_init(const nrf_twi_mngr_t* twi_manager);

// Sets DFR0760 volume: 1-9 (1 lowest, 9 highest)
void DFR0760_set_volume(int volume);

// Send speech synthesis command to DFR0760
void DFR0760_say(const char* text);

// Stop speech synthesis
void DFR0760_stop(void);

// Put DFR0760 into sleep mode
void DFR0760_sleep(void);

// Wake up DFR0760 from sleep mode
void DFR0760_wakeup(void);

// helper function to check if DFR0760 is connected
bool DFR0760_is_connected(const nrf_twi_mngr_t* twi_manager);