#include <stdint.h>

#include "esp_timer.h"

#include "mr24hpc_standard.h"
#include "mr24hpc_state_update.h"
#include "mr24hpc_types.h"

void mr24hpc_standard_handle_frame(uint8_t ctrl, uint8_t cmd, const uint8_t *data, uint16_t len)
{
    if (ctrl != 0x80)
        return;

    uint8_t normalized_cmd = (uint8_t)(cmd & 0x7F);

    if (len < 1)
        return;

    mr24hpc_state_t update = {0};

    switch (normalized_cmd)
    {
    case 0x01:
        update.presence = (data[0] == 0x01) ? MR24HPC_PRESENCE_OCCUPIED : MR24HPC_PRESENCE_UNNOCUPIED;
        update.received_bit_mask = MR24HPC_VALID_PRESENCE;
        break;

    case 0x02:
        if (data[0] == 0x01)
            update.motion = MR24HPC_MOTION_STATIONARY;
        else if (data[0] == 0x02)
            update.motion = MR24HPC_MOTION_ACTIVE;
        else
            update.motion = MR24HPC_MOTION_NONE;
        update.received_bit_mask = MR24HPC_VALID_MOTION;
        break;

    case 0x03:
        update.body_movement = data[0];
        update.received_bit_mask = MR24HPC_VALID_BODY_MOVEMENT;
        break;

    case 0x0B:
        if (data[0] == 0x01)
            update.proximity = MR24HPC_PROXIMITY_NEAR;
        else if (data[0] == 0x02)
            update.proximity = MR24HPC_PROXIMITY_FAR;
        else
            update.proximity = MR24HPC_PROXIMITY_NO_STATE;
        update.received_bit_mask = MR24HPC_VALID_PROXIMITY;
        break;

    default:
        return;
    }

    update.last_update_ms = (uint32_t)(esp_timer_get_time() / 1000ULL);
    mr24hpc_update_state(&update);
}