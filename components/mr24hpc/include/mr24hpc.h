#pragma once

// javni API za MR24HPC driver

#include <stdint.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"

#include "esp_err.h"

#include "mr24hpc_types.h"
#include "mr24hpc_types_uof.h"

typedef void (*state_callback)(const mr24hpc_state_t *state);
typedef void (*heartbeat_callback)(void);

esp_err_t mr24hpc_init(bool UOF_enabled);
esp_err_t mr24hpc_start(void);

esp_err_t register_state_callback(state_callback cb);
esp_err_t register_heartbeat_callback(heartbeat_callback cb);

bool mr24hpc_get_state(mr24hpc_state_t *state_copy);
bool mr24hpc_get_uof_state(UOF_mr24hpc_state_t *state_copy);

bool mr24hpc_is_uof_mode(void);

esp_err_t set_heartbeat_interval_ms(uint32_t interval_ms);
uint32_t get_heartbeat_interval_ms(void);
esp_err_t set_query_interval_ms(uint32_t interval_ms);
uint32_t get_query_interval_ms(void);
