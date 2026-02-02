#pragma once

//javni API za MR24HPC driver

#include <stdint.h>
#include <stdbool.h>

#include "esp_err.h"

#include "mr24hpc_types.h"

//pokazivac na funkciju: (mr24hpc_state_t*) -> void
typedef void (*mr24hpc_callback)(const mr24hpc_state_t *state);

esp_err_t mr24hpc_init(void);
esp_err_t mr24hpc_start(void);
//esp_err_t mr24hpc_stop(void);

bool mr24hpc_get_state(mr24hpc_state_t *state_copy);
esp_err_t mr24hpc_register_callback(mr24hpc_callback cb);
