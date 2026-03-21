#pragma once

#include <stdint.h>

#include "mr24hpc.h"

void sensor_logger_print_standard(const mr24hpc_state_t *state, uint32_t query_interval_ms);
void sensor_logger_print_uof(const UOF_mr24hpc_state_t *state, uint32_t query_interval_ms);
