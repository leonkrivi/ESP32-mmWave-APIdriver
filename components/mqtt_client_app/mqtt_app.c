#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "mqtt_client.h"

#include "mqtt_app.h"

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define MQTT_CONNECTED_BIT (1 << 0)
#define MQTT_RECONNECT_DELAY_MS 5000
#define MQTT_BROKER_URI "mqtt://" MQTT_BROKER_IP ":1883"

static const char *TAG = "* mqtt_app *";

static esp_mqtt_client_handle_t mqtt_client = NULL;
static esp_mqtt_client_config_t mqtt_broker_cfg = {
    .broker.address.uri = MQTT_BROKER_URI,
    .network.reconnect_timeout_ms = MQTT_RECONNECT_DELAY_MS,
};
static EventGroupHandle_t mqtt_event_group = NULL;
static mqtt_rate_change_cb_t rate_change_cb = NULL;
static const char *g_rate_command_topic = NULL;

// ==================== Forward Declarations ====================
static const char *uof_direction_to_str(UOF_mr24hpc_direction_t dir);
static void event_handler(void *handler_args,
                          esp_event_base_t base,
                          int32_t event_id,
                          void *event_data);

// ==================== Personal Header functions ====================
void mqtt_app_start(const char *rate_topic)
{

    g_rate_command_topic = rate_topic;

    mqtt_event_group = xEventGroupCreate();

    ESP_LOGW(TAG, "Connecting to broker: %s", mqtt_broker_cfg.broker.address.uri);

    mqtt_client = esp_mqtt_client_init(&mqtt_broker_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
}

void mqtt_app_publish_state(const char *topic, const mr24hpc_state_t *state)
{
    if (!mqtt_client || !mqtt_app_is_connected() || !state || !topic)
        return;

    char payload[160];
    snprintf(payload, sizeof(payload),
             "{"
             "\"presence\":%u,"
             "\"motion\":%u,"
             "\"body_sign\":%u,"
             "\"direction\":%u"
             "}",
             (unsigned)state->presence,
             (unsigned)state->motion,
             (unsigned)state->body_sign,
             (unsigned)state->direction);

    esp_mqtt_client_publish(mqtt_client, topic, payload, 0, 0, 0);
}

void mqtt_app_publish_uof_state(const char *topic, const UOF_mr24hpc_state_t *state)
{
    if (!mqtt_client || !mqtt_app_is_connected() || !state || !topic)
        return;

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
             uof_direction_to_str(state->direction),
             state->moving_params);

    esp_mqtt_client_publish(mqtt_client, topic, payload, 0, 0, 0);
}

void mqtt_app_register_rate_callback(mqtt_rate_change_cb_t cb)
{
    rate_change_cb = cb;
}

void mqtt_app_wait_connected(TickType_t timeout_ticks)
{
    xEventGroupWaitBits(
        mqtt_event_group,
        MQTT_CONNECTED_BIT,
        pdFALSE,
        pdTRUE,
        timeout_ticks);
}

bool mqtt_app_is_connected(void)
{
    if (!mqtt_event_group)
        return false;
    return (xEventGroupGetBits(mqtt_event_group) & MQTT_CONNECTED_BIT) != 0;
}

// ==================== Local functions ====================
static void event_handler(void *handler_args,
                          esp_event_base_t base,
                          int32_t event_id,
                          void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;

    switch (event_id)
    {
        // case MQTT_EVENT_BEFORE_CONNECT:

    case MQTT_EVENT_CONNECTED:
        xEventGroupSetBits(mqtt_event_group, MQTT_CONNECTED_BIT);
        ESP_LOGW(TAG, "Client connected to broker: %s", mqtt_broker_cfg.broker.address.uri);
        if (g_rate_command_topic)
        {
            msg_id = esp_mqtt_client_subscribe_single(client, g_rate_command_topic, 1);
            ESP_LOGW(TAG, "Subscribing to %s (msg_id=%d)", g_rate_command_topic, msg_id);
        }
        break;

    case MQTT_EVENT_DISCONNECTED:
        xEventGroupClearBits(mqtt_event_group, MQTT_CONNECTED_BIT);
        ESP_LOGW(TAG, "Disconnected, auto-reconnect in %d s...", MQTT_RECONNECT_DELAY_MS / 1000);
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGW(TAG, "Subscribed (check: msg_id=%d)", event->msg_id);
        break;

    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGW(TAG, "Unsubscribed (check: msg_id=%d)", event->msg_id);
        break;

        // case MQTT_EVENT_PUBLISHED:

    case MQTT_EVENT_DATA:
        // topic and data are NOT null-terminated, so we use the provided lengths
        const char *topic = event->topic;
        const char *data = event->data;
        ESP_LOGW(TAG, "Received: %.*s -> %.*s",
                 event->topic_len, topic,
                 event->data_len, data);

        // Check if this is a rate change command
        if (g_rate_command_topic &&
            (size_t)event->topic_len == strlen(g_rate_command_topic) &&
            strncmp(topic, g_rate_command_topic, event->topic_len) == 0)
        {
            // convert 15 character string to uint32_t interval in ms
            char buf[16] = {0};
            int len = event->data_len < 15 ? event->data_len : 15;
            strncpy(buf, data, len);
            uint32_t interval = (uint32_t)atoi(buf);

            ESP_LOGW(TAG, "Rate change command: %lu ms", interval);
            if (rate_change_cb)
            {
                rate_change_cb(interval);
            }
        }
        break;

    default:
        break;
    }
}

static const char *uof_direction_to_str(UOF_mr24hpc_direction_t dir)
{
    switch (dir)
    {
    case UOF_MR24HPC_DIR_APPROACHING:
        return "APPROACHING";
    case UOF_MR24HPC_DIR_MOVING_AWAY:
        return "MOVING AWAY";
    default:
        return "NONE/STATIONARY";
    }
}
