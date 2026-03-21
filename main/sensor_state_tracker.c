#include "sensor_state_tracker.h"

static mr24hpc_state_t g_last_state;
static bool g_has_state;

static UOF_mr24hpc_state_t g_last_uof_state;
static bool g_has_uof_state;

static bool has_state_changed(const mr24hpc_state_t *curr, const mr24hpc_state_t *prev)
{
    return curr->presence != prev->presence ||
           curr->motion != prev->motion ||
           curr->body_sign != prev->body_sign ||
           curr->direction != prev->direction;
}

static bool has_uof_state_changed(const UOF_mr24hpc_state_t *curr, const UOF_mr24hpc_state_t *prev)
{
    return curr->existence_energy != prev->existence_energy ||
           curr->static_distance_m != prev->static_distance_m ||
           curr->motion_energy != prev->motion_energy ||
           curr->motion_distance_m != prev->motion_distance_m ||
           curr->motion_speed_m_s != prev->motion_speed_m_s ||
           curr->direction != prev->direction ||
           curr->moving_params != prev->moving_params;
}

// ==================== Personal Header functions ====================

void sensor_state_tracker_init(void)
{
    g_last_state = (mr24hpc_state_t){0};
    g_has_state = false;
    g_last_uof_state = (UOF_mr24hpc_state_t){0};
    g_has_uof_state = false;
}

sensor_tracker_result_t sensor_state_tracker_evaluate(const mr24hpc_state_t *state)
{
    if (!g_has_state)
    {
        g_last_state = *state;
        g_has_state = true;
        return SENSOR_TRACKER_FIRST_SAMPLE;
    }

    if (!has_state_changed(state, &g_last_state))
        return SENSOR_TRACKER_NO_CHANGE;

    g_last_state = *state;
    return SENSOR_TRACKER_CHANGED;
}

sensor_tracker_result_t sensor_state_tracker_evaluate_uof(const UOF_mr24hpc_state_t *state)
{
    if (!g_has_uof_state)
    {
        g_last_uof_state = *state;
        g_has_uof_state = true;
        return SENSOR_TRACKER_FIRST_SAMPLE;
    }

    if (!has_uof_state_changed(state, &g_last_uof_state))
        return SENSOR_TRACKER_NO_CHANGE;

    g_last_uof_state = *state;
    return SENSOR_TRACKER_CHANGED;
}
