#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "mr24hpc.h"
#include "mr24hpc_state_update.h"

extern SemaphoreHandle_t state_mutex;
extern mr24hpc_state_t g_sensor_state;
extern UOF_mr24hpc_state_t g_uof_sensor_state;
extern mr24hpc_callback g_cb_function;

void mr24hpc_update_state(const mr24hpc_state_t *delta)
{
    if (!delta)
    {
        return;
    }

    mr24hpc_state_lock();

    if (delta->valid_mask & MR24HPC_VALID_PRESENCE)
    {
        g_sensor_state.presence = delta->presence;
    }

    if (delta->valid_mask & MR24HPC_VALID_MOTION)
    {
        g_sensor_state.motion = delta->motion;
    }

    if (delta->valid_mask & MR24HPC_VALID_BODY_SIGN)
    {
        g_sensor_state.body_sign = delta->body_sign;
    }

    if (delta->valid_mask & MR24HPC_VALID_DIRECTION)
    {
        g_sensor_state.direction = delta->direction;
    }

    g_sensor_state.last_update_ms = delta->last_update_ms;
    g_sensor_state.valid_mask = delta->valid_mask;

    mr24hpc_callback cb = g_cb_function;
    mr24hpc_state_t snapshot = g_sensor_state;

    mr24hpc_state_unlock();

    if (cb)
    {
        cb(&snapshot);
    }
}

void mr24hpc_update_uof_state(const UOF_mr24hpc_state_t *delta)
{
    if (!delta)
    {
        return;
    }

    mr24hpc_state_lock();

    if (delta->valid_mask & UOF_MR24HPC_VALID_EXISTENCE_ENERGY)
    {
        g_uof_sensor_state.existence_energy = delta->existence_energy;
    }

    if (delta->valid_mask & UOF_MR24HPC_VALID_STATIC_DISTANCE)
    {
        g_uof_sensor_state.static_distance_m = delta->static_distance_m;
    }

    if (delta->valid_mask & UOF_MR24HPC_VALID_MOTION_ENERGY)
    {
        g_uof_sensor_state.motion_energy = delta->motion_energy;
    }

    if (delta->valid_mask & UOF_MR24HPC_VALID_MOTION_DISTANCE)
    {
        g_uof_sensor_state.motion_distance_m = delta->motion_distance_m;
    }

    if (delta->valid_mask & UOF_MR24HPC_VALID_MOTION_SPEED)
    {
        g_uof_sensor_state.motion_speed_m_s = delta->motion_speed_m_s;
    }

    if (delta->valid_mask & UOF_MR24HPC_VALID_DIRECTION)
    {
        g_uof_sensor_state.direction = delta->direction;
    }

    if (delta->valid_mask & UOF_MR24HPC_VALID_MOVING_PARAMS)
    {
        g_uof_sensor_state.moving_params = delta->moving_params;
    }

    g_uof_sensor_state.last_update_ms = delta->last_update_ms;
    g_uof_sensor_state.valid_mask = delta->valid_mask;

    mr24hpc_callback cb = g_cb_function;
    mr24hpc_state_t snapshot = g_sensor_state;

    mr24hpc_state_unlock();

    if (cb)
    {
        cb(&snapshot);
    }
}

void mr24hpc_state_lock(void)
{
    xSemaphoreTake(state_mutex, portMAX_DELAY);
}

void mr24hpc_state_unlock(void)
{
    xSemaphoreGive(state_mutex);
}