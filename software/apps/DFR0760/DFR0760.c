// DFR0760.c
// Driver for DFR0760 Text-to-Speech Module
// Reference code:
// 1. Lab6 I2C Sensors
// 2. DFRobot_SpeechSynthesis_V2.cpp from DFRobot
// 3. ChatGPT (See inline comments for details)

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "nrf_delay.h"
#include "app_util_platform.h" 
#include "DFR0760.h"

static const nrf_twi_mngr_t* i2c_manager = NULL;

// I2C helper functioned modified to send multiple bytes
// Reference: Lab6 I2C Sensors
static ret_code_t i2c_read_bytes(uint8_t i2c_addr, uint8_t* rx_buf, uint8_t length) {
    if (i2c_manager == NULL) return NRF_ERROR_INVALID_STATE;
    nrf_twi_mngr_transfer_t const read_transfer[] = {
        NRF_TWI_MNGR_READ(i2c_addr, rx_buf, length, 0)
    };
    return nrf_twi_mngr_perform(i2c_manager, NULL, read_transfer, 1, NULL);
}

static ret_code_t i2c_write_bytes(uint8_t i2c_addr, const uint8_t *data, size_t length) {
    if (i2c_manager == NULL) return NRF_ERROR_INVALID_STATE;
    nrf_twi_mngr_transfer_t const write_transfer = NRF_TWI_MNGR_WRITE(i2c_addr, data, length, 0);
    ret_code_t result = nrf_twi_mngr_perform(i2c_manager, NULL, &write_transfer, 1, NULL);
    if (result != NRF_SUCCESS) {  
        //printf("DFR0760: I2C write failed! Error: 0x%lX\n", (unsigned long)result);
    }
    return result;
}

// =======================
// DFR0760 Interface
// Reference Code: https://github.com/DFRobot/DFRobot_SpeechSynthesis_V2/blob/main/DFRobot_SpeechSynthesis_V2.cpp

bool DFR0760_init(const nrf_twi_mngr_t* i2c) {
    i2c_manager = i2c;

    // This module's workflow: 0xAA sequence -> status check -> wait for 0x4F
    // Microbit send a somewhat attention signal to device and check its response before init
    uint8_t sync_byte = 0xAA; // Sync signal
    uint8_t status_check_command[] = {CMD_HEADER, 0x00, 0x01, INQUIRYSTATUS}; // Status Check
    uint8_t ack_val = 0; // acknowledge

    for (int i = 0; i < 40; i++) {
        // Send 0xAA
        if(i2c_write_bytes(DFR0760_ADDR, &sync_byte, 1) != NRF_SUCCESS) {
            nrf_delay_ms(50);
            continue;
        }
        nrf_delay_ms(50);

        // Send status check 
        if (i2c_write_bytes(DFR0760_ADDR, status_check_command, sizeof(status_check_command)) != NRF_SUCCESS) {
            nrf_delay_ms(50);
            continue;
        }
        nrf_delay_ms(20);

        if (i2c_read_bytes(DFR0760_ADDR, &ack_val, 1) == NRF_SUCCESS) {
            if (ack_val == ACK_DEVICE_READY) { // check if we get 0x4F from device
                break;
            }
        } else {
            printf("Initialization Failed. Not reading ACK\n");
        }
    }

    DFR0760_set_volume(5);
    printf("DFR0760 Initialized Sucessfully.\n");
    return true;
}


// I made the whole thing blocking 
void DFR0760_wait_for_speech_to_finish(void) {
    uint8_t status_check_command[] = {CMD_HEADER, 0x00, 0x01, INQUIRYSTATUS}; // Status Check
    uint8_t ack_val = 0; // acknowledgement value from DFR0760

    // Source: ChatGPT (I asked how much polling time to wait for the TTS module to finish speaking)
    const int max_poll_trials = 100;          // poll for 0x41 for ~2s (delay = 20 ms)
    const int max_status_check_trials = 250;  // poll for  ~12.5 seconds (delay = 50ms)

    // Check if synthesis is active (i.e. the device is still speaking)
    for(int attempt_count = 0; attempt_count < max_poll_trials; attempt_count++) {
        if (i2c_read_bytes(DFR0760_ADDR, &ack_val, 1) == NRF_SUCCESS) {
            if (ack_val == ACK_SYNTHESIS_ACTIVE_OR_START) { // 0x41 'A'
                break;
            }
            if (ack_val == ACK_DEVICE_READY) { // Speech is already done or didn't start correctly
              printf("Error: Speech terminated earlier than expected\n");
              return; 
            }
        }
        nrf_delay_ms(20); // short interval for polling
    }

    nrf_delay_ms(100);

    for(int attempt_count = 0; attempt_count < max_status_check_trials; attempt_count++) {
        if(i2c_write_bytes(DFR0760_ADDR, status_check_command, sizeof(status_check_command)) != NRF_SUCCESS){
            nrf_delay_ms(50);
            continue;
        }
        nrf_delay_ms(10); 

        // Reading ACK to be 0x4F 'O'
        if (i2c_read_bytes(DFR0760_ADDR, &ack_val, 1) == NRF_SUCCESS) {
            if (ack_val == ACK_DEVICE_READY) {
                return; // Speech is finished
            }
        }
        nrf_delay_ms(50);
        //printf("Error: Synthesis Time Out");
    }
}

void DFR0760_say(const char *text) {
    uint16_t length = strlen(text);
    if (length == 0) return;
    if (length > 250) {
        printf("Error: Text is too long.\n");
        return;
    }

    // This part is copied from the ref code cuz the packet stuff is kinda complicated 
    /*
    case START_SYNTHESIS: {
    length = 2 + len;
    head[1] = length >> 8;
    head[2] = length & 0xff;
    head[3] = START_SYNTHESIS;
    head[4] = 0x03;
    sendCommand(head, data, len);
  } break;
    */

    // Source: ChatGPT (I asked how to send packet to the TTS module)
    uint8_t head[length + 5]; 
    head[0] = CMD_HEADER;
    // Length field is for (command_byte + encoding_byte + text_data)
    uint16_t payload_len = 2 + length;
    head[1] = (payload_len >> 8) & 0xFF;   // High byte of payload_len
    head[2] = payload_len & 0xFF;          // Low byte of payload_len
    head[3] = START_SYNTHESIS;             // Command for speech synthesis (0x01)
    head[4] = 0x00;                        // Encoding: 0x00 for English (V1 lib)
    
    memcpy(&head[5], text, length);
    //printf("DFR0760: Saying: \"%s\" (len: %u, cmd:0x%02X, enc:0x%02X)\n", text, length, head[3], head[4]);
    if(i2c_write_bytes(DFR0760_ADDR, head, length + 5) == NRF_SUCCESS) {
        DFR0760_wait_for_speech_to_finish();
    }
}

void DFR0760_set_volume(int volume) {
    if (volume < 0) volume = 0;
    if (volume > 9) volume = 9;

    char voice_setting[5];
    voice_setting[0] = '[';
    voice_setting[1] = 'v';
    voice_setting[2] = '0' + volume;
    voice_setting[3] = ']';
    voice_setting[4] = '\0';
    // Format: [vX] where X is the volume digit. like [v9]

    DFR0760_say(voice_setting); // IMPORTANT: DONT DELETE // OR INIT WILL FAIL
}

// Basically copying from the manufacturer's ref code
void DFR0760_stop(void) {
    uint8_t stop_cmd[] = { CMD_HEADER, 0x00, 0x01, STOP_SYNTHESIS };
    i2c_write_bytes(DFR0760_ADDR, stop_cmd, sizeof(stop_cmd));
    nrf_delay_ms(50); 
}

void DFR0760_sleep(void) {
    uint8_t sleep_cmd[] = { CMD_HEADER, 0x00, 0x01, ENTERSAVEELETRI };
    i2c_write_bytes(DFR0760_ADDR, sleep_cmd, sizeof(sleep_cmd));
    nrf_delay_ms(50);
}

void DFR0760_wakeup(void) {
    uint8_t wake_cmd[] = { CMD_HEADER, 0x00, 0x01, WAKEUP };
    i2c_write_bytes(DFR0760_ADDR, wake_cmd, sizeof(wake_cmd));
    nrf_delay_ms(100);
}

bool DFR0760_is_connected(const nrf_twi_mngr_t* i2c) {
    if (i2c == NULL && i2c_manager == NULL) return false;
    const nrf_twi_mngr_t* current_manager = (i2c != NULL) ? i2c : i2c_manager;
    if (current_manager == NULL) return false;

    uint8_t dummy_rx_buf = 0;
    nrf_twi_mngr_transfer_t const xfer[] = {
        NRF_TWI_MNGR_READ(DFR0760_ADDR, &dummy_rx_buf, 1, NRF_TWI_MNGR_NO_STOP) // Send a read request
    };
    return nrf_twi_mngr_perform(current_manager, NULL, xfer, 1, NULL) == NRF_SUCCESS;
}