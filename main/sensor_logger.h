#pragma once

#include <stdint.h>

#include "mr24hpc.h"

// for logging and debugging when seq is being generated in app_main
// void sensor_logger_print_standard_with_seq(const mr24hpc_state_t *state, uint32_t query_interval_ms, uint32_t seq);
void sensor_logger_print_standard(const mr24hpc_state_t *state, uint32_t query_interval_ms);
// for logging and debugging when seq is being generated in app_main
// void sensor_logger_print_uof_with_seq(const UOF_mr24hpc_state_t *state, uint32_t query_interval_ms, uint32_t seq);
void sensor_logger_print_uof(const UOF_mr24hpc_state_t *state, uint32_t query_interval_ms);
