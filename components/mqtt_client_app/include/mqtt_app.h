#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"

#include "mr24hpc.h"
#include "mr24hpc_types_uof.h"

typedef void (*mqtt_rate_change_cb_t)(uint32_t interval_ms);

void mqtt_app_start(const char *rate_topic);

void mqtt_app_publish_state(const char *topic, const mr24hpc_state_t *state);
void mqtt_app_publish_uof_state(const char *topic, const UOF_mr24hpc_state_t *state);

void mqtt_app_register_rate_callback(mqtt_rate_change_cb_t cb);

bool mqtt_app_is_connected(void);
void mqtt_app_wait_connected(TickType_t timeout_ticks);