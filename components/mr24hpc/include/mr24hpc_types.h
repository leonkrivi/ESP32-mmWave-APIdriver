#pragma once

#include <stdint.h>

typedef enum
{
    MR24HPC_PRESENCE_NONE = 0,
    MR24HPC_PRESENCE_SOMEONE = 1,
} mr24hpc_presence_t;

typedef enum
{
    MR24HPC_MOTION_NONE = 0,
    MR24HPC_MOTION_STATIONARY = 1,
    MR24HPC_MOTION_ACTIVE = 2,
} mr24hpc_motion_t;

typedef enum
{
    MR24HPC_DIR_NONE = 0,
    MR24HPC_DIR_APPROACHING = 1,
    MR24HPC_DIR_MOVING_AWAY = 2,
} mr24hpc_direction_t;

typedef struct
{
    mr24hpc_presence_t presence;
    mr24hpc_motion_t motion;
    uint8_t body_sign;
    mr24hpc_direction_t direction;
    uint32_t last_update_ms;
    uint32_t valid_mask;
} mr24hpc_state_t;

typedef enum
{
    MR24HPC_VALID_PRESENCE = (1 << 0),
    MR24HPC_VALID_MOTION = (1 << 1),
    MR24HPC_VALID_BODY_SIGN = (1 << 2),
    MR24HPC_VALID_DIRECTION = (1 << 3),
} mr24hpc_valid_mask_t;