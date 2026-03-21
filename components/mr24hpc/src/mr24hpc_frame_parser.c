#include <stddef.h>
#include <stdint.h>

#include "mr24hpc_frame_parser.h"

// ==================== Forward declarations ====================
static uint8_t calculate_checksum(const uint8_t *data, size_t len);

// ==================== Personal parser functions ====================
void mr24hpc_frame_parser_init(mr24hpc_frame_parser_ctx_t *ctx)
{
    if (!ctx)
        return;

    *ctx = (mr24hpc_frame_parser_ctx_t){0};
    ctx->state = WAIT_H1;
}

// universal feed function wrapper with configurable on_frame callback
void mr24hpc_frame_parser_feed(mr24hpc_frame_parser_ctx_t *ctx, uint8_t byte, mr24hpc_frame_handler_t on_frame_parsed)
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
        ctx->ctrl = byte;
        ctx->checksum_buf[ctx->checksum_len++] = byte;
        ctx->state = WAIT_CMD;
        break;

    case WAIT_CMD:
        ctx->cmd = byte;
        ctx->checksum_buf[ctx->checksum_len++] = byte;
        ctx->state = WAIT_LEN1;
        break;

    case WAIT_LEN1: // first (MSB)
        ctx->len = ((uint16_t)byte << 8) & 0xFF00;
        ctx->checksum_buf[ctx->checksum_len++] = byte;
        ctx->state = WAIT_LEN2;
        break;

    case WAIT_LEN2: // second (LSB)
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
        ctx->checksum_ok = (calculate_checksum(ctx->checksum_buf, ctx->checksum_len) == byte);
        if (ctx->checksum_ok)
            ctx->state = WAIT_END1;
        else
            ctx->state = WAIT_H1;
        break;

    case WAIT_END1:
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
        if (byte == MR24_FRAME_END_2 && ctx->checksum_ok && on_frame_parsed)
            on_frame_parsed(ctx->ctrl, ctx->cmd, ctx->data, ctx->len);
        ctx->state = WAIT_H1;
        break;
    }
}

static uint8_t calculate_checksum(const uint8_t *data, size_t len)
{
    uint32_t sum = 0;

    for (size_t i = 0; i < len; ++i)
    {
        sum += data[i];
    }

    return (uint8_t)(sum & 0xFF);
}
