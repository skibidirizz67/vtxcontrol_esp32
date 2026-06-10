#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "Arduino.h"

#define TRAMP_MIN_BAUD  9120    // -5%
#define TRAMP_BAUD      9600
#define TRAMP_MAX_BAUD  10080   // +5%
#define TRAMP_BAUD_STEP 48      // 0.5%

#define TRAMP_SYNC_START    0x0F
#define TRAMP_SYNC_STOP     0x00

#define TRAMP_CMD_SET_FREQ      0x46 // 'F' -- set frequency
#define TRAMP_CMD_SET_POW       0x50 // 'P' -- set power
#define TRAMP_CMD_ACTIVE_STATE  0x49 // 'I' -- (apparently) set pitmode?
#define TRAMP_CMD_GET_CONFIG    0x76 // 'v' -- get settings/config/parameters request
#define TRAMP_CMD_CMD_RF        0x72 // 'r' -- init/reset/activate Tramp?
#define TRAMP_CMD_CMD_SENSOR    0x73 // 's' -- get temperature

#define TRAMP_FRAME_SIZE    16
#define TRAMP_PLD_SIZE      12

#define TRAMP_MAX_RETRIES 16

#define TRAMP_RSP_TIMEOUT_MS 1000

#define TRAMP_MIN_REQUEST_PERIOD_US     200_000  // 200ms
#define TRAMP_STATUS_REQUEST_PERIOD_US  1000_000 // 1s

#define VTX_Serial Serial2

typedef struct {
    uint16_t freq;
    uint16_t pow;
    uint8_t control_mode;
    uint8_t pit_mode;
    uint16_t act_pow;
} TrampParams;

typedef union {
    uint8_t buf[TRAMP_PLD_SIZE];
    TrampParams params;
    uint16_t freq, pow, temp, param;
    uint8_t active;
} TrampPld;

typedef struct {
    uint8_t sync_start;
    uint8_t cmd;
    TrampPld pld;
    uint8_t crc;
    uint8_t sync_stop;
} TrampFrame;

extern TrampParams params_buf;
extern uint16_t temp_buf;

bool tramp_init();
bool tramp_update();
void tramp_clear_uart();