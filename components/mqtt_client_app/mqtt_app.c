#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_log.h"

#include "mqtt_app.h"
#include "mqtt_event.h"
#include "mqtt_types.h"

static const char *TAG = "* mqtt_app *";

static mqtt_app_context_t g_ctx = {
    .client = NULL,
    .client_cfg = {
        .broker.address.uri = MQTT_BROKER_URI,
        .network.reconnect_timeout_ms = MQTT_RECONNECT_DELAY_MS,
        .session.disable_clean_session = true,
        .session.keepalive = MQTT_KEEPALIVE_SEC,
        .session.disable_keepalive = false,
        .session.protocol_ver = MQTT_PROTOCOL_V_5,
        .session.last_will = {
            .topic = NULL,
            .msg = "{\"status\":\"offline\"}",
            .qos = 1,
            .retain = true,
        },
    },
    .event_group = NULL,
    .hb_rate_change_cb = NULL,
    .sensor_rate_change_cb = NULL,
    .configuration_topic = NULL,
    .sensor_status_topic = NULL,
    .sensor_status_check_topic = NULL,
    .state_payload_seq = 0,
    .sensor_status_payload_seq = 0,
};

// ==================== Forward Declarations ====================

static uint32_t next_state_payload_seq(void);
static uint32_t next_sensor_status_payload_seq(void);
static const char *uof_direction_to_str(UOF_mr24hpc_direction_t dir);

// ==================== Personal Header functions ====================
void mqtt_app_start(const char *device_id,
                    const char *room_id,
                    const char *connection_status_topic,
                    const char *configuration_topic,
                    const char *sensor_status_topic,
                    const char *sensor_status_check_topic)
{
    g_ctx.device_id = device_id;
    g_ctx.room_id = room_id;

    g_ctx.connection_status_topic = connection_status_topic;
    g_ctx.configuration_topic = configuration_topic;
    g_ctx.sensor_status_topic = sensor_status_topic;
    g_ctx.sensor_status_check_topic = sensor_status_check_topic;

    g_ctx.client_cfg.session.last_will.topic = connection_status_topic;

    g_ctx.state_payload_seq = 0;
    g_ctx.sensor_status_payload_seq = 0;

    if (!g_ctx.event_group)
        g_ctx.event_group = xEventGroupCreate();

    if (!g_ctx.event_group)
    {
        ESP_LOGE(TAG, "Failed to create MQTT event group");
        return;
    }

    ESP_LOGW(TAG, "Connecting to broker: %s", g_ctx.client_cfg.broker.address.uri);

    g_ctx.client = esp_mqtt_client_init(&g_ctx.client_cfg);
    if (!g_ctx.client)
    {
        ESP_LOGE(TAG, "Failed to initialize MQTT client");
        return;
    }

#ifdef CONFIG_MQTT_PROTOCOL_5
    esp_mqtt5_connection_property_config_t connect_props = {
        .session_expiry_interval = MQTT_SESSION_EXPIRY_SEC,
        .will_delay_interval = MQTT_LWT_WILL_DELAY_SEC,
    };

    if (esp_mqtt5_client_set_connect_property(g_ctx.client, &connect_props) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set MQTT5 connect properties");
        return;
    }
#endif

    esp_mqtt_client_register_event(g_ctx.client, ESP_EVENT_ANY_ID, mqtt_app_event_handler, &g_ctx);
    esp_mqtt_client_start(g_ctx.client);
}

void mqtt_app_register_rate_callbacks(mqtt_hb_rate_change_cb_t hb_cb,
                                      mqtt_sensor_rate_change_cb_t sensor_cb)
{
    g_ctx.hb_rate_change_cb = hb_cb;
    g_ctx.sensor_rate_change_cb = sensor_cb;
}

void mqtt_app_publish_sensor_status(const char *topic, const char *status, uint32_t hb_interval)
{
    if (!g_ctx.client || !mqtt_app_is_connected() || !status || !topic)
        return;

    uint32_t seq = next_sensor_status_payload_seq();

    char payload[192];
    snprintf(payload, sizeof(payload),
             "{"
             "\"seq\":%" PRIu32 ","
             "\"sensor\":\"%s\","
             "\"hb_interval\": \"%u\""
             "}",
             seq,
             status,
             (unsigned)hb_interval);

    esp_mqtt_client_publish(g_ctx.client, topic, payload, 0, 0, 0); // QoS 0
}

void mqtt_app_publish_state(const char *topic, const mr24hpc_state_t *state)
{
    if (!g_ctx.client || !mqtt_app_is_connected() || !state || !topic)
        return;

    uint32_t seq = next_state_payload_seq();

    char payload[192];
    snprintf(payload, sizeof(payload),
             "{"
             "\"seq\":%" PRIu32 ","
             "\"presence\":%u,"
             "\"motion\":%u,"
             "\"sensor_rate\":%u"
             "}",
             seq,
             (unsigned)state->presence,
             (unsigned)state->motion,
             (unsigned)get_sensor_rate_interval_ms());

    esp_mqtt_client_publish(g_ctx.client, topic, payload, 0, 0, 0); // QoS 0
}

// currently out of date, needs to be updated to match the non-UOF state struct and fields
void mqtt_app_publish_uof_state(const char *topic, const UOF_mr24hpc_state_t *state)
{
    if (!g_ctx.client || !mqtt_app_is_connected() || !state || !topic)
        return;

    uint32_t seq = next_state_payload_seq();

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
static uint32_t next_state_payload_seq(void)
{
    return __atomic_fetch_add(&g_ctx.state_payload_seq, 1U, __ATOMIC_RELAXED);
}

static uint32_t next_sensor_status_payload_seq(void)
{
    return __atomic_fetch_add(&g_ctx.sensor_status_payload_seq, 1U, __ATOMIC_RELAXED);
}