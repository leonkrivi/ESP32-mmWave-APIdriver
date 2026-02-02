#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_log.h"
#include "mqtt_client.h"

#include "mqtt_app.h"

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

// MQTT topics
#define MQTT_TOPIC_STATE     "/esp32/sensor/state"
#define MQTT_TOPIC_RATE_CMD  "/esp32/sensor/rate"

ESP_EVENT_DEFINE_BASE(MQTT_APP_USER_EVENT_BASE);

static const char *TAG ="* mqtt_app *";

static esp_mqtt_client_handle_t mqtt_client = NULL;
static esp_mqtt_client_config_t mqtt_broker_cfg = {
        .broker.address.uri = "mqtt://" MQTT_BROKER_IP ":1883" // From .env file, loaded with CMake
};
static int retry_num = 0;
static bool connected = false;
static mqtt_rate_change_cb_t rate_change_cb = NULL;

// Helper: convert direction enum to string
static const char* dir_to_str(mr24hpc_direction_t dir) {
    switch (dir) {
        case MR24HPC_DIR_APPROACHING: return "approaching";
        case MR24HPC_DIR_MOVING_AWAY: return "moving_away";
        default: return "none";
    }
}


static void user_event_handler(void *handler_args,
                               esp_event_base_t event_base,
                               int32_t event_id,
                               void *event_data)
{
    if (event_id == MQTT_APP_USER_EVENT_STOP && mqtt_client) {
        esp_mqtt_client_stop(mqtt_client);
        esp_mqtt_client_destroy(mqtt_client);
        mqtt_client = NULL;
        connected = false;
    }
}


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
            if (retry_num < 1)
                ESP_LOGW(TAG, "Client initialized, connecting...");
            break;

        case MQTT_EVENT_CONNECTED:
            retry_num = 0;
            connected = true;
            ESP_LOGW(TAG, "Client connected to broker: %s", mqtt_broker_cfg.broker.address.uri);
            ESP_LOGW(TAG, "Subscribing to topic %s", MQTT_TOPIC_RATE_CMD);
            msg_id = esp_mqtt_client_subscribe_single(client, MQTT_TOPIC_RATE_CMD, 1);
            ESP_LOGW(TAG, "Subscribed to %s (msg_id=%d)", MQTT_TOPIC_RATE_CMD, msg_id);
            break;

        case MQTT_EVENT_DISCONNECTED:
            connected = false;
            if (retry_num < 3) {
                retry_num++;
                uint32_t delay_ms = retry_num * 3000;
                ESP_LOGW(TAG, "Client disconnected, reconnecting...");
                vTaskDelay(pdMS_TO_TICKS(delay_ms));
                esp_mqtt_client_reconnect(client);
            } 
            else {
                retry_num++;
                ESP_LOGE(TAG, "Client disconnected, timeout reached");
                ESP_LOGW(TAG, "Stopping MQTT client");
                ESP_ERROR_CHECK(esp_event_post(MQTT_APP_USER_EVENT_BASE, MQTT_APP_USER_EVENT_STOP, NULL, 0, portMAX_DELAY));
            }
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGW(TAG, "Subscribed (check: msg_id=%d)", event->msg_id);
            break;

        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGW(TAG, "Unsubscribed (check: msg_id=%d)", event->msg_id);
            break;

        case MQTT_EVENT_PUBLISHED:
            break;

        case MQTT_EVENT_DATA:
            ESP_LOGW(TAG, "Received: %.*s -> %.*s", 
                event->topic_len, event->topic,
                event->data_len, event->data);

            // Check if this is a rate change command
            if (strncmp(event->topic, MQTT_TOPIC_RATE_CMD, event->topic_len) == 0) {
                char buf[16] = {0};
                int len = event->data_len < 15 ? event->data_len : 15;
                strncpy(buf, event->data, len);
                uint32_t interval = (uint32_t)atoi(buf);
                
                ESP_LOGW(TAG, "Rate change command: %lu ms", interval);
                if (rate_change_cb) {
                    rate_change_cb(interval);
                }
            }
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

    ESP_LOGW(TAG, "Connecting to broker: %s", mqtt_broker_cfg.broker.address.uri); 

    mqtt_client = esp_mqtt_client_init(&mqtt_broker_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, event_handler, NULL);
    esp_event_handler_register(MQTT_APP_USER_EVENT_BASE, ESP_EVENT_ANY_ID, user_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);

    return;
}

void mqtt_app_publish_state(const mr24hpc_state_t *state) {
    if (!mqtt_client || !connected || !state) return;

    // JSON payload
    char payload[256];
    snprintf(payload, sizeof(payload),
        "{"
        "\"existence_energy\":%u,"
        "\"static_distance_m\":%.1f,"
        "\"motion_energy\":%u,"
        "\"motion_distance_m\":%.1f,"
        "\"motion_speed_m_s\":%.1f,"
        "\"direction\":\"%s\","
        "\"moving_params\":%u"
        "}",
        state->existence_energy,
        state->static_distance_m,
        state->motion_energy,
        state->motion_distance_m,
        state->motion_speed_m_s,
        dir_to_str(state->direction),
        state->moving_params
    );

    esp_mqtt_client_publish(mqtt_client, MQTT_TOPIC_STATE, payload, 0, 0, 0);
}

void mqtt_app_register_rate_callback(mqtt_rate_change_cb_t cb) {
    rate_change_cb = cb;
}

bool mqtt_app_is_connected(void) {
    return connected;
}