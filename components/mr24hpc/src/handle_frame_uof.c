#include <stdint.h>

#include "esp_timer.h"

#include "mr24hpc_uof.h"
#include "mr24hpc_state_update.h"
#include "mr24hpc_types_uof.h"

static float UOF_static_dist_to_m(uint8_t code);
static float UOF_motion_dist_to_m(uint8_t code);
static float UOF_motion_speed_to_m_s(uint8_t code);
static UOF_mr24hpc_direction_t UOF_speed_to_direction(uint8_t code);

void uof_handle_frame(uint8_t ctrl, uint8_t cmd, const uint8_t *data, uint16_t len)
{
    if (ctrl != 0x08)
        return;

    UOF_mr24hpc_state_t update = {0};

    switch (cmd)
    {
    case 0x01:
        if (len < 5)
            return;

        update.existence_energy = data[0];
        update.static_distance_m = UOF_static_dist_to_m(data[1]);
        update.motion_energy = data[2];
        update.motion_distance_m = UOF_motion_dist_to_m(data[3]);
        update.motion_speed_m_s = UOF_motion_speed_to_m_s(data[4]);
        update.direction = UOF_speed_to_direction(data[4]);
        update.received_bit_mask = UOF_MR24HPC_VALID_EXISTENCE_ENERGY |
                                   UOF_MR24HPC_VALID_STATIC_DISTANCE |
                                   UOF_MR24HPC_VALID_MOTION_ENERGY |
                                   UOF_MR24HPC_VALID_MOTION_DISTANCE |
                                   UOF_MR24HPC_VALID_MOTION_SPEED |
                                   UOF_MR24HPC_VALID_DIRECTION;
        break;

    case 0x81:
        if (len < 1)
            return;
        update.existence_energy = data[0];
        update.received_bit_mask = UOF_MR24HPC_VALID_EXISTENCE_ENERGY;
        break;

    case 0x82:
        if (len < 1)
            return;
        update.motion_energy = data[0];
        update.received_bit_mask = UOF_MR24HPC_VALID_MOTION_ENERGY;
        break;

    case 0x83:
        if (len < 1)
            return;
        update.static_distance_m = UOF_static_dist_to_m(data[0]);
        update.received_bit_mask = UOF_MR24HPC_VALID_STATIC_DISTANCE;
        break;

    case 0x84:
        if (len < 1)
            return;
        update.motion_distance_m = UOF_motion_dist_to_m(data[0]);
        update.received_bit_mask = UOF_MR24HPC_VALID_MOTION_DISTANCE;
        break;

    case 0x85:
        if (len < 1)
            return;
        update.motion_speed_m_s = UOF_motion_speed_to_m_s(data[0]);
        update.direction = UOF_speed_to_direction(data[0]);
        update.received_bit_mask = UOF_MR24HPC_VALID_MOTION_SPEED | UOF_MR24HPC_VALID_DIRECTION;
        break;

    case 0x86:
        if (len < 1)
            return;
        update.direction = (UOF_mr24hpc_direction_t)data[0];
        update.received_bit_mask = UOF_MR24HPC_VALID_DIRECTION;
        break;

    case 0x87:
        if (len < 1)
            return;
        update.moving_params = data[0];
        update.received_bit_mask = UOF_MR24HPC_VALID_MOVING_PARAMS;
        break;

    default:
        return;
    }

    update.last_update_ms = (uint32_t)(esp_timer_get_time() / 1000ULL);
    update_uof_state(&update);
}

static float UOF_static_dist_to_m(uint8_t code)
{
    if (code == 0x00)
        return 0.0f;

    return code * 0.5f;
}

static float UOF_motion_dist_to_m(uint8_t code)
{
    if (code == 0x00)
        return 0.0f;

    return code * 0.5f;
}

static float UOF_motion_speed_to_m_s(uint8_t code)
{
    if (code == 0x00 || code == 0x0A)
        return 0.0f;

    if (code >= 0x01 && code <= 0x09)
        return (0x0A - code) * 0.5f;

    if (code >= 0x0B && code <= 0x14)
        return -((int)(code - 0x0A)) * 0.5f;

    return 0.0f;
}

static UOF_mr24hpc_direction_t UOF_speed_to_direction(uint8_t code)
{
    if (code >= 0x01 && code <= 0x09)
        return UOF_MR24HPC_DIR_APPROACHING;

    if (code >= 0x0B && code <= 0x14)
        return UOF_MR24HPC_DIR_MOVING_AWAY;

    return UOF_MR24HPC_DIR_NONE;
}