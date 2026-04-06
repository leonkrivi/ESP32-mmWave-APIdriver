#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "mr24hpc.h"
#include "mr24hpc_types_uof.h"

// shared internal globals defined in mr24hpc.c and used across mr24hpc module sources.
extern SemaphoreHandle_t state_mutex;
extern mr24hpc_state_t g_sensor_state;
extern UOF_mr24hpc_state_t g_uof_sensor_state;
extern state_callback g_state_callback;
extern heartbeat_callback g_heartbeat_callback;

void update_state(const mr24hpc_state_t *delta);
void update_uof_state(const UOF_mr24hpc_state_t *delta);

void state_lock(void);
void state_unlock(void);