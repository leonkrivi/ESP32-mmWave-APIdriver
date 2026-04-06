#pragma once

#include <stdint.h>

#include "freertos/event_groups.h"
#include "mqtt_client.h"

#include "mqtt_app.h"

#define MQTT_CONNECTED_BIT (1 << 0)
#define MQTT_RECONNECT_DELAY_MS 5000
#define MQTT_BROKER_URI "mqtt://" MQTT_BROKER_IP ":1883"
#define MQTT_MAX_CMD_PAYLOAD_LEN 31

typedef struct
{
    esp_mqtt_client_handle_t client;
    esp_mqtt_client_config_t broker_cfg;
    EventGroupHandle_t event_group;

    const char *device_id;
    const char *room;
    mqtt_rate_change_cb_t rate_change_cb;
    void (*connection_check_cb)(void);

    const char *rate_command_topic;
    const char *connection_status_topic;

    uint32_t payload_seq;
} mqtt_app_context_t;

void mqtt_app_event_handler(void *handler_args,
                            esp_event_base_t base,
                            int32_t event_id,
                            void *event_data);
