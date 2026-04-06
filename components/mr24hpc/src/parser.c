#include "mr24hpc_parser.h"
#include "mr24hpc.h"
#include "mr24hpc_standard.h"
#include "mr24hpc_uof.h"

extern heartbeat_callback g_heartbeat_callback;

// ==================== Personal Header functions ====================

void parser_init(mr24hpc_parser_config_t *cfg, bool uof_mode)
{
    if (!cfg)
        return;

    frame_parser_init(&cfg->frame_ctx);
    cfg->on_frame_parsed = uof_mode ? uof_handle_frame : standard_handle_frame;
    cfg->on_heartbeat_detected = NULL;
}

void parser_feed(mr24hpc_parser_config_t *cfg, uint8_t byte)
{
    if (!cfg)
        return;

    frame_parser_feed(&cfg->frame_ctx, byte, cfg->on_frame_parsed, g_heartbeat_callback);
}
