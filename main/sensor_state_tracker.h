#pragma once

#include <stdbool.h>

#include "mr24hpc.h"

typedef enum
{
    SENSOR_TRACKER_NO_CHANGE = 0,
    SENSOR_TRACKER_FIRST_SAMPLE,
    SENSOR_TRACKER_CHANGED
} sensor_tracker_result_t;

void sensor_state_tracker_init(void);

sensor_tracker_result_t sensor_state_tracker_evaluate(const mr24hpc_state_t *state);
sensor_tracker_result_t sensor_state_tracker_evaluate_uof(const UOF_mr24hpc_state_t *state);
