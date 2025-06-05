#include "seg7.h"
#include "app_timer.h"
#include "nrf_delay.h" 
#include <stdio.h>
#include <stdbool.h>

// Commands from SparkFun Serial7SegmentDisplay Wiki
// Reference: https://github.com/sparkfun/Serial7SegmentDisplay/wiki/Special-Commands#decimal
#define SEG7_CMD_CLEAR_DISPLAY  0x76 
#define SEG7_CMD_DECIMAL_CTRL  0x77 
#define SEG7_CMD_CURSOR_CTRL  0x79

// Bitmask for controlling decimals/colon/apostrophe with command 0x77
// Bit:   7(msb) 6   5(')   4(:)   3(DP1) 2(DP2) 1(DP3) 0(DP4) 
// Wiki:  X      X   Apos   Colon  Dec3   Dec2   Dec1   Dec0
// For MM:SS, we want the Colon (Bit 4)
#define SEG7_COLON_BIT            (1 << 4) // Bit 4 for Colon (0b00010000 or 0x10)
#define SEG7_NO_DECIMALS_COLON    0x00     // To turn off all decimals/colon

#define SEG7_UPDATE_INTERVAL  APP_TIMER_TICKS(1000)

APP_TIMER_DEF(seg7_timer);

static const nrf_twi_mngr_t* twi_mngr = NULL;
static volatile uint32_t countdown = 300; //defaule 5 min
static volatile bool colon_blink = true;
static bool debug = true;
static bool blink_mode = true;

// Helper function to send a command with one data byte
static void i2c_write(uint8_t cmd, uint8_t data) {
    if (!twi_mngr) return;
    uint8_t cmd_buf[2] = {cmd, data};
    nrf_twi_mngr_transfer_t xfer = NRF_TWI_MNGR_WRITE(SEG7_I2C_ADDR, cmd_buf, sizeof(cmd_buf), 0);
    nrf_twi_mngr_perform(twi_mngr, NULL, &xfer, 1, NULL);
    nrf_delay_ms(2); // Small delay after cmd
}

// Helper function to send a single byte cmd
static void i2c_send_cmd(uint8_t cmd) {
    if (!twi_mngr) return;
    nrf_twi_mngr_transfer_t xfer = NRF_TWI_MNGR_WRITE(SEG7_I2C_ADDR, &cmd, 1, 0);
    nrf_twi_mngr_perform(twi_mngr, NULL, &xfer, 1, NULL);
    nrf_delay_ms(2); // Small delay after cmd
}

// Consolidated function to update display digits and colon
static void seg7_update_display(uint32_t total_sec, bool show_colon) {
    if (!twi_mngr) return;
    i2c_send_cmd(SEG7_CMD_CLEAR_DISPLAY);

    // 2. Set Colon State
    if (show_colon) {
        i2c_write(SEG7_CMD_DECIMAL_CTRL, SEG7_COLON_BIT);
    } else {
        i2c_write(SEG7_CMD_DECIMAL_CTRL, SEG7_NO_DECIMALS_COLON);
    }

    uint32_t minutes = total_sec / 60;
    uint32_t secs = total_sec % 60;

    char packet[5]; // packet: MMSS\0
    packet[0] = (minutes / 10) + '0';
    packet[1] = (minutes % 10) + '0';
    packet[2] = (secs / 10) + '0';
    packet[3] = (secs % 10) + '0';
    packet[4] = '\0';

    nrf_twi_mngr_transfer_t xfer_digits = NRF_TWI_MNGR_WRITE(SEG7_I2C_ADDR, (uint8_t*)packet, 4, 0);
    nrf_twi_mngr_perform(twi_mngr, NULL, &xfer_digits, 1, NULL);
}


static void seg7_timer_handler(void* context) {
    bool curr_colon_state;

    if (countdown > 0) {
        countdown--;
        if (blink_mode) colon_blink = !colon_blink; // Toggle for blinking
        curr_colon_state = colon_blink;
    } else {
        colon_blink = true; // Reset for next potential countdown
        curr_colon_state = true; // Keep colon ON when at 00:00
    }
    seg7_update_display(countdown, curr_colon_state);
}

void seg7_init(const nrf_twi_mngr_t* i2c, int countdown_sec, bool debug_mode) {
    twi_mngr = i2c;
    debug = debug_mode;
    countdown = countdown_sec;
    // send clear command to reset cursor
    i2c_send_cmd(SEG7_CMD_CLEAR_DISPLAY);

    // Initialize & start timer for countdown
    APP_ERROR_CHECK(app_timer_create(&seg7_timer, APP_TIMER_MODE_REPEATED, seg7_timer_handler));
    APP_ERROR_CHECK(app_timer_start(seg7_timer, SEG7_UPDATE_INTERVAL, NULL));

    colon_blink = true;
    seg7_update_display(countdown, colon_blink);
}

void seg7_set_countdown(uint32_t sec) {
    app_timer_stop(seg7_timer);
    countdown = sec;
    colon_blink = true;
    seg7_update_display(countdown, colon_blink);
    if (sec > 0 || SEG7_UPDATE_INTERVAL > 0) {
         APP_ERROR_CHECK(app_timer_start(seg7_timer, SEG7_UPDATE_INTERVAL, NULL));
    }
}

void seg7_add_sec(uint32_t delta) {
    app_timer_stop(seg7_timer);
    countdown += delta;
    colon_blink = true;
    seg7_update_display(countdown, colon_blink);
    APP_ERROR_CHECK(app_timer_start(seg7_timer, SEG7_UPDATE_INTERVAL, NULL));
}

void seg7_sub_sec(uint32_t delta) {
    app_timer_stop(seg7_timer);
    if (delta >= countdown) {
        countdown = 0;
    } else {
        countdown -= delta;
    }
    colon_blink = true;
    seg7_update_display(countdown, colon_blink);
    if (countdown > 0 || SEG7_UPDATE_INTERVAL > 0) {
        APP_ERROR_CHECK(app_timer_start(seg7_timer, SEG7_UPDATE_INTERVAL, NULL));
    } else if (countdown == 0) {
        seg7_update_display(countdown, true); // Force colon on for 00:00
    }
}

uint32_t seg7_get_countdown(void) {
    return countdown;
}

bool time_runs_out(void) {
    if (countdown == 0) {return true;}
    return false;
}