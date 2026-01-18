#pragma once

#include <stdio.h>

#include "esp_log.h"
#include "mqtt_client.h"

#include "mqtt_app.h"

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

const char *mqtt_broker_ip = STR(MQTT_BROKER_IP); // From .env file, loaded with CMake

static const char *TAG ="* mqtt_app *";
static int retry_num = 0;

static void event_handler(void *handler_args,
                          esp_event_base_t base,
                          int32_t event_id,
                          void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;

    switch (event_id) {
        case MQTT_EVENT_BEFORE_CONNECT:
            ESP_LOGW(TAG, "Client initialized, connecting...");
            break;
        case MQTT_EVENT_CONNECTED:
            retry_num = 0;
            ESP_LOGW(TAG, "Client connected to the broker");
            ESP_LOGW(TAG, "Subscribing to topic /esp32/test");
            msg_id = esp_mqtt_client_subscribe_single(client, "/esp32/test", 0);
            ESP_LOGW(TAG, "Subscription request sent, msg_id=%d", msg_id);
            ESP_LOGW(TAG, "Publishing to topic /esp32/test");
            msg_id = esp_mqtt_client_publish(client, "/esp32/test", "Hello from ESP32 MQTT client", 0, 1, 0);
            ESP_LOGW(TAG, "Message published, msg_id=%d", msg_id);
            break;
        case MQTT_EVENT_DISCONNECTED:
            if (retry_num < 5) {
                ESP_LOGW(TAG, "Client disconnected, reconnecting...");
                esp_mqtt_client_reconnect(client);
                retry_num++;
            } else {
                ESP_LOGE(TAG, "Client disconnected, timeout reached");
            }
            break;
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGW(TAG, "Subscribed successfully (check: msg_id=%d)", event->msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGW(TAG, "Unsubscribed successfully (check: msg_id=%d)", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGW(TAG, "Message published successfully (check: msg_id=%d)", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGW(TAG, "Message received on topic: %.*s", event->topic_len, event->topic);
            ESP_LOGW(TAG, "| -- data: %.*s", event->data_len, event->data);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT event error occurred");
            ESP_LOGE(TAG, "Error type: %d", event->error_handle->error_type);
            break;
        default:
            ESP_LOGE(TAG, "Unknown event id:%d", event_id);
            break;
    }
}


void mqtt_app_start(void) {

    esp_mqtt_client_config_t cfg = {
        .broker.address.uri = "mqtt://"MQTT_BROKER_IP":1883",
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, event_handler, NULL);
    esp_mqtt_client_start(client);

    return;
}

void mqtt_app_publish_state(const mr24hpc_state_t *state) {};