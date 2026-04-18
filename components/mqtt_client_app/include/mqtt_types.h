#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "mqtt_client.h"

#define MQTT_CONNECTED_BIT (1 << 0)
#define MQTT_RECONNECT_DELAY_MS 100

#define MQTT_KEEPALIVE_SEC 10
#define MQTT_LWT_WILL_DELAY_SEC 5 // gives X seconds for the recconect
#define MQTT_SESSION_EXPIRY_SEC 3600

#define MQTT_BROKER_URI "mqtt://" MQTT_BROKER_IP ":1883" // loaded at compile time from .env file

typedef void (*mqtt_hb_rate_change_cb_t)(uint32_t interval_ms);
typedef void (*mqtt_sensor_rate_change_cb_t)(uint32_t interval_ms);

typedef struct
{
    esp_mqtt_client_handle_t client;
    esp_mqtt_client_config_t client_cfg;

    EventGroupHandle_t event_group;

    const char *device_id;
    const char *room_id;
    mqtt_hb_rate_change_cb_t hb_rate_change_cb;
    mqtt_sensor_rate_change_cb_t sensor_rate_change_cb;

    const char *connection_status_topic;
    const char *configuration_topic;
    const char *sensor_status_topic;
    const char *sensor_status_check_topic;
    const char *reset_topic;
    bool reset_message_sent;

    uint32_t state_payload_seq;
    uint32_t sensor_status_payload_seq;
} mqtt_app_context_t;
