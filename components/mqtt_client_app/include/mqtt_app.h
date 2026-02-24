#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"

#include "mr24hpc.h"

typedef void (*mqtt_rate_change_cb_t)(uint32_t interval_ms);

void mqtt_app_start(void);
void mqtt_app_wait_connected(TickType_t timeout_ticks);
void mqtt_app_publish_state(const mr24hpc_state_t *state);
void mqtt_app_register_rate_callback(mqtt_rate_change_cb_t cb);
bool mqtt_app_is_connected(void);