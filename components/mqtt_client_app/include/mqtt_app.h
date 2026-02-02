#pragma once

#include "esp_event.h"

#include "mr24hpc.h"

ESP_EVENT_DECLARE_BASE(MQTT_APP_USER_EVENT_BASE);

typedef enum {
    MQTT_APP_USER_EVENT_STOP
} mqtt_app_event_id_t;

typedef void (*mqtt_rate_change_cb_t)(uint32_t interval_ms);

void mqtt_app_start(void);
void mqtt_app_publish_state(const mr24hpc_state_t *state);
void mqtt_app_register_rate_callback(mqtt_rate_change_cb_t cb);
bool mqtt_app_is_connected(void);