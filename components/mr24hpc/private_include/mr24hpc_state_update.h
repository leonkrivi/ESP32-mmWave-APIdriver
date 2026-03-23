#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "mr24hpc.h"
#include "mr24hpc_types_uof.h"

// shared internal globals defined in mr24hpc.c and used across mr24hpc module sources.
extern SemaphoreHandle_t state_mutex;
extern mr24hpc_state_t g_sensor_state;
extern UOF_mr24hpc_state_t g_uof_sensor_state;
extern mr24hpc_callback g_cb_function;

void mr24hpc_update_state(const mr24hpc_state_t *delta);
void mr24hpc_update_uof_state(const UOF_mr24hpc_state_t *delta);

void mr24hpc_state_lock(void);
void mr24hpc_state_unlock(void);