#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "mr24hpc_frame_parser.h"

static uint8_t calculate_checksum(const uint8_t *data, size_t len)
{
    uint32_t sum = 0;

    for (size_t i = 0; i < len; ++i)
    {
        sum += data[i];
    }

    return (uint8_t)(sum & 0xFF);
}

void frame_parser_init(mr24hpc_frame_parser_ctx_t *ctx)
{
    if (!ctx)
        return;

    *ctx = (mr24hpc_frame_parser_ctx_t){0};
    ctx->state = WAIT_H1;
}

// universal feed function wrapper with configurable on_frame_parsed callback
void frame_parser_feed(mr24hpc_frame_parser_ctx_t *ctx,
                       uint8_t byte,
                       mr24hpc_frame_handler_t on_frame_parsed,
                       mr24hpc_heartbeat_handler_t on_heartbeat_detected)
{
    if (!ctx)
        return;

    switch (ctx->state)
    {
    case WAIT_H1:
        if (byte == MR24_FRAME_HEADER_1)
        {
            ctx->checksum_len = 0;
            ctx->checksum_buf[ctx->checksum_len++] = byte;
            ctx->checksum_ok = false;
            ctx->state = WAIT_H2;
            ctx->is_hb = true; // optimistically assume it's a heartbeat until we see otherwise
        }
        break;

    case WAIT_H2:
        if (byte == MR24_FRAME_HEADER_2)
        {
            ctx->checksum_buf[ctx->checksum_len++] = byte;
            ctx->state = WAIT_CTRL;
        }
        else if (byte == MR24_FRAME_HEADER_1)
        {
            ctx->checksum_len = 0;
            ctx->checksum_buf[ctx->checksum_len++] = byte;
            ctx->state = WAIT_H2;
        }
        else
            ctx->state = WAIT_H1;
        break;

    case WAIT_CTRL:
        if (byte != 0x01)
            ctx->is_hb = false;
        ctx->ctrl = byte;
        ctx->checksum_buf[ctx->checksum_len++] = byte;
        ctx->state = WAIT_CMD;
        break;

    case WAIT_CMD:
        if (byte != 0x01)
            ctx->is_hb = false;
        ctx->cmd = byte;
        ctx->checksum_buf[ctx->checksum_len++] = byte;
        ctx->state = WAIT_LEN1;
        break;

    case WAIT_LEN1: // first (MSB)
        if (byte != 0x00)
            ctx->is_hb = false;
        ctx->len = ((uint16_t)byte << 8) & 0xFF00;
        ctx->checksum_buf[ctx->checksum_len++] = byte;
        ctx->state = WAIT_LEN2;
        break;

    case WAIT_LEN2: // second (LSB)
        if (byte != 0x01)
            ctx->is_hb = false;
        ctx->len = (ctx->len & 0xFF00) | (uint16_t)byte;
        ctx->checksum_buf[ctx->checksum_len++] = byte;
        ctx->data_idx = 0;

        if (ctx->len == 0)
            ctx->state = WAIT_CHECKSUM;
        else if (ctx->len <= (uint16_t)sizeof(ctx->data))
            ctx->state = WAIT_DATA;
        else
            ctx->state = WAIT_H1;
        break;

    case WAIT_DATA:
        if (byte != 0x0F)
            ctx->is_hb = false;
        if (ctx->data_idx < (uint16_t)sizeof(ctx->data))
        {
            ctx->data[ctx->data_idx++] = byte;
            ctx->checksum_buf[ctx->checksum_len++] = byte;
        }
        else
        {
            ctx->state = WAIT_H1;
            break;
        }

        if (ctx->data_idx >= ctx->len)
            ctx->state = WAIT_CHECKSUM;
        break;

    case WAIT_CHECKSUM:
        if (byte != 0xBE)
            ctx->is_hb = false;
        ctx->checksum_ok = (calculate_checksum(ctx->checksum_buf, ctx->checksum_len) == byte);
        if (ctx->checksum_ok)
            ctx->state = WAIT_END1;
        else
            ctx->state = WAIT_H1;
        break;

    case WAIT_END1:
        if (byte != 0x54)
            ctx->is_hb = false;
        if (byte == MR24_FRAME_END_1)
            ctx->state = WAIT_END2;
        else if (byte == MR24_FRAME_HEADER_1)
        {
            ctx->checksum_len = 0;
            ctx->checksum_buf[ctx->checksum_len++] = byte;
            ctx->checksum_ok = false;
            ctx->state = WAIT_H2;
        }
        else
            ctx->state = WAIT_H1;
        break;

    case WAIT_END2:
        if (byte != 0x43)
            ctx->is_hb = false;
        if (byte == MR24_FRAME_END_2 && ctx->checksum_ok)
        {
            if (on_frame_parsed)
                on_frame_parsed(ctx->ctrl, ctx->cmd, ctx->data, ctx->len);
            if (ctx->is_hb && on_heartbeat_detected)
                on_heartbeat_detected();
        }
        ctx->state = WAIT_H1;
        break;
    }
}
