#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "mr24hpc.h"
#include "mr24hpc_state_update.h"

void update_state(const mr24hpc_state_t *delta)
{
    if (!delta)
        return;

    state_lock();

    if (delta->received_bit_mask & MR24HPC_VALID_PRESENCE)
        g_sensor_state.presence = delta->presence;

    if (delta->received_bit_mask & MR24HPC_VALID_MOTION)
        g_sensor_state.motion = delta->motion;

    if (delta->received_bit_mask & MR24HPC_VALID_BODY_MOVEMENT)
        g_sensor_state.body_movement = delta->body_movement;

    if (delta->received_bit_mask & MR24HPC_VALID_PROXIMITY)
        g_sensor_state.proximity = delta->proximity;

    g_sensor_state.last_update_ms = delta->last_update_ms;
    g_sensor_state.received_bit_mask = delta->received_bit_mask;

    state_callback cb = g_state_callback;
    mr24hpc_state_t snapshot = g_sensor_state;

    state_unlock();

    if (cb)
        cb(&snapshot);
}

void update_uof_state(const UOF_mr24hpc_state_t *delta)
{
    if (!delta)
        return;

    state_lock();

    if (delta->received_bit_mask & UOF_MR24HPC_VALID_EXISTENCE_ENERGY)
        g_uof_sensor_state.existence_energy = delta->existence_energy;

    if (delta->received_bit_mask & UOF_MR24HPC_VALID_STATIC_DISTANCE)
        g_uof_sensor_state.static_distance_m = delta->static_distance_m;

    if (delta->received_bit_mask & UOF_MR24HPC_VALID_MOTION_ENERGY)
        g_uof_sensor_state.motion_energy = delta->motion_energy;

    if (delta->received_bit_mask & UOF_MR24HPC_VALID_MOTION_DISTANCE)
        g_uof_sensor_state.motion_distance_m = delta->motion_distance_m;

    if (delta->received_bit_mask & UOF_MR24HPC_VALID_MOTION_SPEED)
        g_uof_sensor_state.motion_speed_m_s = delta->motion_speed_m_s;

    if (delta->received_bit_mask & UOF_MR24HPC_VALID_DIRECTION)
        g_uof_sensor_state.direction = delta->direction;

    if (delta->received_bit_mask & UOF_MR24HPC_VALID_MOVING_PARAMS)
        g_uof_sensor_state.moving_params = delta->moving_params;

    g_uof_sensor_state.last_update_ms = delta->last_update_ms;
    g_uof_sensor_state.received_bit_mask = delta->received_bit_mask;

    state_callback cb = g_state_callback;
    mr24hpc_state_t snapshot = g_sensor_state;

    state_unlock();

    if (cb)
        cb(&snapshot);
}

void state_lock(void)
{
    xSemaphoreTake(state_mutex, portMAX_DELAY);
}

void state_unlock(void)
{
    xSemaphoreGive(state_mutex);
}