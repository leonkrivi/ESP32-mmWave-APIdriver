#pragma once

#include <stdint.h>

// user manual: https://files.seeedstudio.com/wiki/mmWave-radar/MR24HPC1_User_Manual-V2.0.pdf

typedef enum
{
    MR24HPC_PRESENCE_UNNOCUPIED = 0,
    MR24HPC_PRESENCE_OCCUPIED = 1,
} mr24hpc_presence_t;

typedef enum
{
    MR24HPC_MOTION_NONE = 0,
    MR24HPC_MOTION_STATIONARY = 1,
    MR24HPC_MOTION_ACTIVE = 2,
} mr24hpc_motion_t;

typedef enum
{
    MR24HPC_PROXIMITY_NO_STATE = 0, // no one present, person still or random motion
    MR24HPC_PROXIMITY_NEAR = 1,     // approaching the radar continuously for 3 seconds
    MR24HPC_PROXIMITY_FAR = 2,      // moving away from the radar continuously for 3 seconds
} mr24hpc_proximity_t;

// ==================== Sensor State Structure ====================
typedef struct
{
    mr24hpc_presence_t presence;
    mr24hpc_motion_t motion;
    uint8_t body_movement; // 0-100 scale (no one present 0, person still 1, person active 25)
    mr24hpc_proximity_t proximity;

    uint32_t last_update_ms;
    uint32_t received_bit_mask;
} mr24hpc_state_t;

typedef enum
{
    MR24HPC_VALID_PRESENCE = (1 << 0),
    MR24HPC_VALID_MOTION = (1 << 1),
    MR24HPC_VALID_BODY_MOVEMENT = (1 << 2),
    MR24HPC_VALID_PROXIMITY = (1 << 3),
} mr24hpc_valid_mask_t;