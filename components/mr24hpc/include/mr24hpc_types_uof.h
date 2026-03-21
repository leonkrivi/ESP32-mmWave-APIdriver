#pragma once

#include <stdint.h>

// Motion speed direction enum (UOF payload semantics)
typedef enum
{
    UOF_MR24HPC_DIR_NONE = 0,        // no movement / stationary
    UOF_MR24HPC_DIR_APPROACHING = 1, // approaching radar
    UOF_MR24HPC_DIR_MOVING_AWAY = 2, // moving away from radar
} UOF_mr24hpc_direction_t;

typedef struct
{
    // Existence (static presence)
    uint8_t existence_energy; // 0-250, electromagnetic wave reflection strength
    float static_distance_m;  // stationary person distance (0.5m steps, max 3m)

    // Motion
    uint8_t motion_energy;             // 0-250, motion amplitude value
    float motion_distance_m;           // moving target distance (0.5m steps, max 4m)
    float motion_speed_m_s;            // speed in m/s (0.5m/s steps)
    UOF_mr24hpc_direction_t direction; // approaching / moving away / none

    // Moving parameters (0-100)
    uint8_t moving_params;

    // Timestamps
    uint32_t last_update_ms; // timestamp of last update
    uint32_t valid_mask;     // bitmask of which fields are valid (changed)
} UOF_mr24hpc_state_t;

typedef enum
{
    UOF_MR24HPC_VALID_EXISTENCE_ENERGY = (1 << 0),
    UOF_MR24HPC_VALID_STATIC_DISTANCE = (1 << 1),
    UOF_MR24HPC_VALID_MOTION_ENERGY = (1 << 2),
    UOF_MR24HPC_VALID_MOTION_DISTANCE = (1 << 3),
    UOF_MR24HPC_VALID_MOTION_SPEED = (1 << 4),
    UOF_MR24HPC_VALID_DIRECTION = (1 << 5),
    UOF_MR24HPC_VALID_MOVING_PARAMS = (1 << 6),
} UOF_mr24hpc_valid_mask_t;
