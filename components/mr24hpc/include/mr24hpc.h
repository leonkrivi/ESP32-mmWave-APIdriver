#pragma once

// javni API za MR24HPC driver

#include <stdint.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"

#include "esp_err.h"

#include "mr24hpc_types.h"
#include "mr24hpc_types_uof.h"

typedef void (*mr24hpc_callback)(const mr24hpc_state_t *state);

esp_err_t mr24hpc_init(bool UOF_enabled);
esp_err_t mr24hpc_start(void);
// esp_err_t mr24hpc_stop(void);

esp_err_t mr24hpc_register_callback(mr24hpc_callback cb);

bool mr24hpc_get_state(mr24hpc_state_t *state_copy);
bool mr24hpc_get_uof_state(UOF_mr24hpc_state_t *state_copy);

bool mr24hpc_is_uof_mode(void);

esp_err_t mr24hpc_set_query_interval_ms(uint32_t interval_ms);
uint32_t mr24hpc_get_query_interval_ms(void);
