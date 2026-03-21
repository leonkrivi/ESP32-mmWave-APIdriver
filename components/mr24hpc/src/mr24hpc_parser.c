#include "mr24hpc_parser.h"
#include "mr24hpc_standard.h"
#include "mr24hpc_uof.h"

// ==================== Personal Header functions ====================

void mr24hpc_parser_init(mr24hpc_parser_config_t *cfg, bool uof_mode)
{
    if (!cfg)
        return;

    mr24hpc_frame_parser_init(&cfg->frame_ctx);
    cfg->on_frame_parsed = uof_mode ? mr24hpc_uof_handle_frame : mr24hpc_standard_handle_frame;
}

void mr24hpc_parser_feed(mr24hpc_parser_config_t *cfg, uint8_t byte)
{
    if (!cfg)
        return;

    mr24hpc_frame_parser_feed(&cfg->frame_ctx, byte, cfg->on_frame_parsed);
}
