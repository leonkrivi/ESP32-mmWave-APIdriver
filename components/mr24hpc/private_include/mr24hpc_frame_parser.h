#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define MR24_FRAME_HEADER_1 0x53
#define MR24_FRAME_HEADER_2 0x59
#define MR24_FRAME_END_1 0x54
#define MR24_FRAME_END_2 0x43

#define MR24_FRAME_MAX_DATA_LEN 32
#define MR24_FRAME_MAX_CHECKSUM_BUF 256

typedef enum
{
    WAIT_H1,
    WAIT_H2,
    WAIT_CTRL,
    WAIT_CMD,
    WAIT_LEN1, // first (MSB)
    WAIT_LEN2, // second (LSB)
    WAIT_DATA,
    WAIT_CHECKSUM,
    WAIT_END1,
    WAIT_END2,
} mr24hpc_frame_parser_state_t;

typedef struct
{
    mr24hpc_frame_parser_state_t state;
    uint8_t ctrl;
    uint8_t cmd;
    uint16_t len;
    uint16_t data_idx;
    uint8_t data[MR24_FRAME_MAX_DATA_LEN];
    uint8_t checksum_buf[MR24_FRAME_MAX_CHECKSUM_BUF];
    size_t checksum_len;
    bool checksum_ok;
    bool is_hb;
} mr24hpc_frame_parser_ctx_t;

typedef void (*mr24hpc_frame_handler_t)(uint8_t ctrl, uint8_t cmd, const uint8_t *data, uint16_t len);
typedef void (*mr24hpc_heartbeat_handler_t)(void);

void frame_parser_init(mr24hpc_frame_parser_ctx_t *ctx);
void frame_parser_feed(mr24hpc_frame_parser_ctx_t *ctx, uint8_t byte, mr24hpc_frame_handler_t on_frame_parsed, mr24hpc_heartbeat_handler_t on_heartbeat_detected);
