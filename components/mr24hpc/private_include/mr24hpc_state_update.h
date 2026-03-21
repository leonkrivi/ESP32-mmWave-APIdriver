#pragma once

#include "mr24hpc_types.h"
#include "mr24hpc_types_uof.h"

void mr24hpc_update_state(const mr24hpc_state_t *delta);
void mr24hpc_update_uof_state(const UOF_mr24hpc_state_t *delta);

void mr24hpc_state_lock(void);
void mr24hpc_state_unlock(void);