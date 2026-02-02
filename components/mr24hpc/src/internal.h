#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "mr24hpc_types.h"
#include "mr24hpc_uart.h"

typedef enum {
    WAIT_H1,
    WAIT_H2,
    WAIT_CTRL,
    WAIT_CMD,
    WAIT_LEN1, // first (MSB)
    WAIT_LEN2, // second (LSB)
    WAIT_DATA,
    WAIT_CHECKSUM
} mr24hpc_parser_state_t;

typedef struct {
    mr24hpc_parser_state_t ps;
    uint8_t ctrl;
    uint8_t cmd;
    uint16_t len; // 16-bit length
    uint8_t data[32];
    uint16_t data_idx;
} mr24hpc_parser_t;

// Interni API drivera
void mr24hpc_parser_init();
void mr24hpc_parser_feed(uint8_t byte);

void mr24hpc_update_state(const mr24hpc_state_t *delta);
void handle_frame(uint8_t ctrl, uint8_t cmd, const uint8_t *data, uint16_t len);

void mr24hpc_state_lock(void);
void mr24hpc_state_unlock(void);

// Global sensor state (defined in mr24hpc.c)
extern mr24hpc_state_t global_sensor_state;

// pomocne funkcije
uint8_t calculate_checksum(const uint8_t *data, size_t len);
uint32_t mr24hpc_ms_since_last_update(void);