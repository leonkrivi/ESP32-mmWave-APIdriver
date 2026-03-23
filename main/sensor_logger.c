#include "sensor_logger.h"

#include <stdio.h>

#include "esp_timer.h"

static uint32_t g_standard_log_seq = 0;
static uint32_t g_uof_log_seq = 0;

static uint32_t get_time_ms(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000ULL);
}

static const char *presence_to_str(mr24hpc_presence_t presence)
{
    return (presence == MR24HPC_PRESENCE_OCCUPIED) ? "OCCUPIED" : "UNOCCUPIED";
}

static const char *motion_to_str(mr24hpc_motion_t motion)
{
    switch (motion)
    {
    case MR24HPC_MOTION_STATIONARY:
        return "STATIONARY";
    case MR24HPC_MOTION_ACTIVE:
        return "ACTIVE";
    default:
        return "NONE";
    }
}

static const char *proximity_to_str(mr24hpc_proximity_t proximity)
{
    switch (proximity)
    {
    case MR24HPC_PROXIMITY_NEAR:
        return "NEAR";
    case MR24HPC_PROXIMITY_FAR:
        return "FAR";
    case MR24HPC_PROXIMITY_NO_STATE:
        return "NO_STATE";
    default:
        return "UNKNOWN";
    }
}

static const char *uof_direction_to_str(UOF_mr24hpc_direction_t dir)
{
    switch (dir)
    {
    case UOF_MR24HPC_DIR_APPROACHING:
        return "APPROACHING";
    case UOF_MR24HPC_DIR_MOVING_AWAY:
        return "MOVING AWAY";
    default:
        return "NONE/STATIONARY";
    }
}

void sensor_logger_print_standard(const mr24hpc_state_t *state, uint32_t query_interval_ms)
{

    printf("\n========== MR24HPC Sensor Update ==========\n");
    printf("(query interval: %lu ms)\n", query_interval_ms);
    printf("Presence:           %s\n", presence_to_str(state->presence));
    printf("Motion:             %s\n", motion_to_str(state->motion));
    // printf("Body Movement:      %u\n", state->body_movement);
    // printf("Proximity:          %s\n", proximity_to_str(state->proximity));
    printf("Age:                %lu ms\n", get_time_ms() - state->last_update_ms);
    printf("============================================\n\n");
}

void sensor_logger_print_uof(const UOF_mr24hpc_state_t *state, uint32_t query_interval_ms)
{

    printf("\n========== MR24HPC Sensor Update (UOF) ==========\n");
    printf("(query interval: %lu ms)\n", query_interval_ms);
    printf("Existence Energy:   %u\n", state->existence_energy);
    printf("Static Distance:    %.1f m\n", state->static_distance_m);
    printf("Motion Energy:      %u\n", state->motion_energy);
    printf("Motion Distance:    %.1f m\n", state->motion_distance_m);
    printf("Motion Speed:       %.1f m/s\n", state->motion_speed_m_s);
    printf("Direction:          %s\n", uof_direction_to_str(state->direction));
    printf("Moving Params:      %u\n", state->moving_params);
    printf("Age:                %lu ms\n", get_time_ms() - state->last_update_ms);
    printf("==================================================\n\n");
}
