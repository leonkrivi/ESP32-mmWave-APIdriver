#pragma once

#include "mr24hpc.h"

void mqtt_app_start(void);
void mqtt_app_publish_state(const mr24hpc_state_t *state);
