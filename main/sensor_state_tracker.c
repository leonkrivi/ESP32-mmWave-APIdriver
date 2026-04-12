#include "sensor_state_tracker.h"

#define STANDARD_CYCLE_EXPECTED_MASK (MR24HPC_VALID_PRESENCE | MR24HPC_VALID_MOTION | MR24HPC_VALID_BODY_MOVEMENT | MR24HPC_VALID_PROXIMITY)
#define STANDARD_CYCLE_TIMEOUT_MS 1200U
#define UOF_CYCLE_EXPECTED_MASK (UOF_MR24HPC_VALID_EXISTENCE_ENERGY | UOF_MR24HPC_VALID_STATIC_DISTANCE | UOF_MR24HPC_VALID_MOTION_ENERGY | UOF_MR24HPC_VALID_MOTION_DISTANCE | UOF_MR24HPC_VALID_MOTION_SPEED | UOF_MR24HPC_VALID_DIRECTION | UOF_MR24HPC_VALID_MOVING_PARAMS)
#define UOF_CYCLE_TIMEOUT_MS 1200U

static uint32_t g_standard_cycle_mask;
static uint32_t g_standard_cycle_start_ms;
static bool g_standard_cycle_started;

static uint32_t g_uof_cycle_mask;
static uint32_t g_uof_cycle_start_ms;
static bool g_uof_cycle_started;

static mr24hpc_state_t g_last_state;
static bool g_has_state;

static UOF_mr24hpc_state_t g_last_uof_state;
static bool g_has_uof_state;

static bool has_state_changed(const mr24hpc_state_t *curr, const mr24hpc_state_t *prev)
{
    return curr->presence != prev->presence ||
           curr->motion != prev->motion;
    //        curr->body_movement != prev->body_movement ||
    //        curr->proximity != prev->proximity;
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

static void begin_standard_cycle(uint32_t start_ms)
{
    g_standard_cycle_started = true;
    g_standard_cycle_start_ms = start_ms;
    g_standard_cycle_mask = 0;
}

static void clear_standard_cycle(void)
{
    g_standard_cycle_started = false;
    g_standard_cycle_start_ms = 0;
    g_standard_cycle_mask = 0;
}

static void begin_uof_cycle(uint32_t start_ms)
{
    g_uof_cycle_started = true;
    g_uof_cycle_start_ms = start_ms;
    g_uof_cycle_mask = 0;
}

static void clear_uof_cycle(void)
{
    g_uof_cycle_started = false;
    g_uof_cycle_start_ms = 0;
    g_uof_cycle_mask = 0;
}

// ==================== Personal Header functions ====================

void sensor_state_tracker_init(void)
{
    g_last_state = (mr24hpc_state_t){0};
    g_has_state = false;
    clear_standard_cycle();
    g_last_uof_state = (UOF_mr24hpc_state_t){0};
    g_has_uof_state = false;
    clear_uof_cycle();
}

sensor_tracker_result_t sensor_state_tracker_evaluate(const mr24hpc_state_t *state)
{
    if (!state)
        return SENSOR_TRACKER_NO_CHANGE;

    if (!g_has_state)
    {
        g_last_state = *state;
        g_has_state = true;
        return SENSOR_TRACKER_FIRST_SAMPLE;
    }

    // 1 maintain standard cycle window (start or timeout recovery).
    if (!g_standard_cycle_started)
        begin_standard_cycle(state->last_update_ms);
    else if ((uint32_t)(state->last_update_ms - g_standard_cycle_start_ms) > STANDARD_CYCLE_TIMEOUT_MS)
        // recovery path for a missed frame: start a fresh cycle window.
        begin_standard_cycle(state->last_update_ms);

    // 2 accumulate incoming field bits, defer decision until cycle is complete.
    g_standard_cycle_mask |= state->received_bit_mask;
    if ((g_standard_cycle_mask & STANDARD_CYCLE_EXPECTED_MASK) != STANDARD_CYCLE_EXPECTED_MASK)
        return SENSOR_TRACKER_CYCLE_INCOMPLETE;

    // 3 full cycle snapshot is ready, close cycle and evaluate state change.
    clear_standard_cycle();

    bool changed = has_state_changed(state, &g_last_state);
    g_last_state = *state;
    return changed ? SENSOR_TRACKER_CHANGED : SENSOR_TRACKER_NO_CHANGE;
}

sensor_tracker_result_t sensor_state_tracker_evaluate_uof(const UOF_mr24hpc_state_t *state)
{
    if (!state)
        return SENSOR_TRACKER_NO_CHANGE;

    if (!g_has_uof_state)
    {
        g_last_uof_state = *state;
        g_has_uof_state = true;
        return SENSOR_TRACKER_FIRST_SAMPLE;
    }

    // 1 maintain UOF cycle window (start or timeout recovery).
    if (!g_uof_cycle_started)
        begin_uof_cycle(state->last_update_ms);
    else if ((uint32_t)(state->last_update_ms - g_uof_cycle_start_ms) > UOF_CYCLE_TIMEOUT_MS)
        // recovery path for a missed frame: start a fresh cycle window.
        begin_uof_cycle(state->last_update_ms);

    // 2 accumulate incoming field bits, defer decision until cycle is complete.
    g_uof_cycle_mask |= state->received_bit_mask;
    if ((g_uof_cycle_mask & UOF_CYCLE_EXPECTED_MASK) != UOF_CYCLE_EXPECTED_MASK)
        return SENSOR_TRACKER_CYCLE_INCOMPLETE;

    // 3 full cycle snapshot is ready, close cycle and evaluate state change.
    clear_uof_cycle();

    bool changed = has_uof_state_changed(state, &g_last_uof_state);
    g_last_uof_state = *state;
    return changed ? SENSOR_TRACKER_CHANGED : SENSOR_TRACKER_NO_CHANGE;
}
