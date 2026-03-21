#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "mr24hpc_types.h"
#include "mr24hpc_frame_parser.h"

typedef struct
{
    mr24hpc_frame_parser_ctx_t frame_ctx;
    mr24hpc_frame_handler_t on_frame_parsed;
} mr24hpc_parser_config_t;

void mr24hpc_parser_init(mr24hpc_parser_config_t *cfg, bool uof_mode);
void mr24hpc_parser_feed(mr24hpc_parser_config_t *cfg, uint8_t byte);