#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_log.h"

#include "mqtt_app.h"
#include "mqtt_event.h"

static const char *TAG = "* mqtt_app *";

static mqtt_app_context_t g_ctx = {
    .client = NULL,
    .broker_cfg = {
        .broker.address.uri = MQTT_BROKER_URI,
        .network.reconnect_timeout_ms = MQTT_RECONNECT_DELAY_MS,
    },
    .event_group = NULL,
    .rate_change_cb = NULL,
    .connection_check_cb = NULL,
    .rate_command_topic = NULL,
    .connection_status_topic = NULL,
    .payload_seq = 0,
};

// ==================== Forward Declarations ====================
static const char *uof_direction_to_str(UOF_mr24hpc_direction_t dir);
static uint32_t mqtt_next_payload_seq(void);

// ==================== Personal Header functions ====================
void mqtt_app_start(const char *device_id, const char *room, const char *rate_topic, const char *connection_status_topic)
{
    g_ctx.device_id = device_id;
    g_ctx.room = room;
    g_ctx.rate_command_topic = rate_topic;
    g_ctx.connection_status_topic = connection_status_topic;
    g_ctx.payload_seq = 0;

    if (!g_ctx.event_group)
        g_ctx.event_group = xEventGroupCreate();

    if (!g_ctx.event_group)
    {
        ESP_LOGE(TAG, "Failed to create MQTT event group");
        return;
    }

    ESP_LOGW(TAG, "Connecting to broker: %s", g_ctx.broker_cfg.broker.address.uri);

    g_ctx.client = esp_mqtt_client_init(&g_ctx.broker_cfg);
    if (!g_ctx.client)
    {
        ESP_LOGE(TAG, "Failed to initialize MQTT client");
        return;
    }

    esp_mqtt_client_register_event(g_ctx.client, ESP_EVENT_ANY_ID, mqtt_app_event_handler, &g_ctx);
    esp_mqtt_client_start(g_ctx.client);
}

void mqtt_app_publish_state(const char *topic, const mr24hpc_state_t *state)
{
    if (!g_ctx.client || !mqtt_app_is_connected() || !state || !topic)
        return;

    uint32_t seq = mqtt_next_payload_seq();

    char payload[192];
    snprintf(payload, sizeof(payload),
             "{"
             "\"room\":\"%s\","
             "\"device_id\":\"%s\","
             "\"seq\":%" PRIu32 ","
             "\"presence\":%u,"
             "\"motion\":%u"
             //  "\"body_movement\":%u,"
             //  "\"proximity\":%u"
             "}",
             g_ctx.room,
             g_ctx.device_id,
             seq,
             (unsigned)state->presence,
             (unsigned)state->motion);
    //  (unsigned)state->body_movement,
    //  (unsigned)state->proximity

    esp_mqtt_client_publish(g_ctx.client, topic, payload, 0, 0, 0);
}

// currently out of date, needs to be updated to match the non-UOF state struct and fields
void mqtt_app_publish_uof_state(const char *topic, const UOF_mr24hpc_state_t *state)
{
    if (!g_ctx.client || !mqtt_app_is_connected() || !state || !topic)
        return;

    uint32_t seq = mqtt_next_payload_seq();

    char payload[288];
    snprintf(payload, sizeof(payload),
             "{"
             "\"seq\":%" PRIu32 ","
             "\"existence_energy\":%u,"
             "\"static_distance_m\":%.1f,"
             "\"motion_energy\":%u,"
             "\"motion_distance_m\":%.1f,"
             "\"motion_speed_m_s\":%.1f,"
             "\"direction\":\"%s\","
             "\"moving_params\":%u"
             "}",
             seq,
             state->existence_energy,
             state->static_distance_m,
             state->motion_energy,
             state->motion_distance_m,
             state->motion_speed_m_s,
             uof_direction_to_str(state->direction),
             state->moving_params);

    esp_mqtt_client_publish(g_ctx.client, topic, payload, 0, 0, 0);
}

void mqtt_app_register_rate_callback(mqtt_rate_change_cb_t cb)
{
    g_ctx.rate_change_cb = cb;
}

void mqtt_app_register_connection_check_callback(void (*cb)(void))
{
    g_ctx.connection_check_cb = cb;
}

void mqtt_app_wait_connected(TickType_t timeout_ticks)
{
    if (!g_ctx.event_group)
        return;

    xEventGroupWaitBits(
        g_ctx.event_group,
        MQTT_CONNECTED_BIT,
        pdFALSE,
        pdTRUE,
        timeout_ticks);
}

bool mqtt_app_is_connected(void)
{
    if (!g_ctx.event_group)
        return false;
    return (xEventGroupGetBits(g_ctx.event_group) & MQTT_CONNECTED_BIT) != 0;
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

// thread-safe equivalent to g_payload_seq++
static uint32_t mqtt_next_payload_seq(void)
{
    return __atomic_fetch_add(&g_ctx.payload_seq, 1U, __ATOMIC_RELAXED);
}
