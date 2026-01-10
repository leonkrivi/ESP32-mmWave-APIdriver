#pragma once

#include <stdio.h>

#include "esp_log.h"
#include "mqtt_client.h"

#include "mqtt_app.h"


static const char *TAG ="mqtt_app";
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
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "Mqtt connected");
            msg_id = esp_mqtt_client_subscribe_single(client, "/topic/test", 0);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
            break;

        case MQTT_EVENT_DISCONNECTED:
            if (retry_num < 5) {
                ESP_LOGW(TAG, "Mqtt disconnected, retrying...\n");
                esp_mqtt_client_reconnect(client);
                retry_num++;
            } else {
                ESP_LOGE(TAG, "Mqtt disconnected, timeout reached\n");
            }
            break;

        /*
        case MQTT_EVENT_SUBSCRIBED:
            printf("MQTT_EVENT_SUBSCRIBED, msg_id=%d\n", event->msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            printf("MQTT_EVENT_UNSUBSCRIBED, msg_id=%d\n", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            printf("MQTT_EVENT_PUBLISHED, msg_id=%d\n", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            printf("MQTT_EVENT_DATA\n");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
            break;
        case MQTT_EVENT_ERROR:
            printf("MQTT_EVENT_ERROR\n");
            break;
        default:
            printf("Other event id:%d\n", event_id);
            break;
        */
    }
}


void mqtt_app_start(void) {

    esp_mqtt_client_config_t cfg = {
        .broker.address.uri = "mqtt://broker.hivemq.com",
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, event_handler, NULL);
    esp_mqtt_client_start(client);

    return;
}

void mqtt_app_publish_state(const mr24hpc_state_t *state) {};