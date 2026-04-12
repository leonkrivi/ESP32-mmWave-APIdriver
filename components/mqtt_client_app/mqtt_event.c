#include <stdbool.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "esp_log.h"

#include "mr24hpc.h"
#include "mqtt_event.h"

static const char *TAG = "* mqtt_app *";

// ==================== Forward Declarations ====================

static void process_configuration_message(mqtt_app_context_t *ctx,
                                          const char *topic,
                                          size_t topic_len,
                                          const char *data,
                                          int data_len);
static void process_check_connection_status_message(mqtt_app_context_t *ctx,
                                                    const char *topic,
                                                    size_t topic_len,
                                                    const char *data,
                                                    int data_len);
static bool topic_matches(const char *topic, size_t topic_len, const char *expected_topic);
static bool copy_payload(char *dst, size_t dst_size, const char *data, int d_len);
static bool parse_u32_decimal(const char *text, uint32_t *value_out);
static bool extract_json_rate_value(const char *json,
                                    const char *key,
                                    uint32_t *value_out);

// ==================== Event Handler ====================

void mqtt_app_event_handler(void *handler_args,
                            esp_event_base_t base,
                            int32_t event_id,
                            void *event_data)
{
    (void)base;

    mqtt_app_context_t *ctx = (mqtt_app_context_t *)handler_args;
    esp_mqtt_event_handle_t event = event_data;

    if (!ctx || !event)
        return;

    esp_mqtt_client_handle_t client = event->client;

    switch (event_id)
    {
    case MQTT_EVENT_CONNECTED:
        if (ctx->event_group)
            xEventGroupSetBits(ctx->event_group, MQTT_CONNECTED_BIT);
        ESP_LOGW(TAG, "Client connected to broker: %s", ctx->client_cfg.broker.address.uri);

        // publish retained "online" message to LWT topic upon successful connection
        esp_mqtt_client_publish(client, ctx->connection_status_topic, "{\"status\":\"online\"}", 0, 1, 1); // QoS 1, retain=true

        if (ctx->configuration_topic)
            esp_mqtt_client_subscribe_single(client, ctx->configuration_topic, 1); // QoS 1
        if (ctx->sensor_status_check_topic)
            esp_mqtt_client_subscribe_single(client, ctx->sensor_status_check_topic, 1); // QoS 1
        break;

    case MQTT_EVENT_DISCONNECTED:
        if (ctx->event_group)
            xEventGroupClearBits(ctx->event_group, MQTT_CONNECTED_BIT);
        ESP_LOGW(TAG, "Disconnected, auto-reconnect in %d ms...", MQTT_RECONNECT_DELAY_MS);
        break;

    case MQTT_EVENT_DATA:
    {
        const char *topic = event->topic;
        const char *data = event->data;
        int t_len = event->topic_len;
        int d_len = event->data_len;

        if (!topic || !data)
            break;

        ESP_LOGI(TAG, "received message: (%.*s) %.*s", t_len, topic, d_len, data);

        process_configuration_message(ctx, topic, (size_t)t_len, data, d_len);
        process_check_connection_status_message(ctx, topic, (size_t)t_len, data, d_len);
        break;
    }

    default:
        break;
    }
}

static void process_configuration_message(mqtt_app_context_t *ctx,
                                          const char *topic,
                                          size_t topic_len,
                                          const char *data,
                                          int data_len)
{
    if (!ctx || !topic_matches(topic, topic_len, ctx->configuration_topic))
        return;

    char payload[192];
    if (!copy_payload(payload, sizeof(payload), data, data_len))
        return;

    uint32_t hb_rate_ms = 0;
    if (extract_json_rate_value(payload, "hb_rate", &hb_rate_ms) && hb_rate_ms > 0)
    {
        if (ctx->hb_rate_change_cb)
        {
            ESP_LOGI(TAG, "--> received hb rate change command: %" PRIu32 " ms", hb_rate_ms);
            ctx->hb_rate_change_cb(hb_rate_ms);
        }
    }

    uint32_t sensor_rate_ms = 0;
    if (extract_json_rate_value(payload, "sensor_rate", &sensor_rate_ms) && sensor_rate_ms > 0)
    {
        if (ctx->sensor_rate_change_cb)
        {
            ESP_LOGI(TAG, "--> received sensor rate change command: %" PRIu32 " ms", sensor_rate_ms);
            ctx->sensor_rate_change_cb(sensor_rate_ms);
        }
    }
}

static void process_check_connection_status_message(mqtt_app_context_t *ctx,
                                                    const char *topic,
                                                    size_t topic_len,
                                                    const char *data,
                                                    int data_len)
{
    if (!ctx || !topic_matches(topic, topic_len, ctx->sensor_status_check_topic))
        return;

    char payload[192];
    if (!copy_payload(payload, sizeof(payload), data, data_len) || strcmp(payload, "{\"check sensor\"}") != 0)
        return;

    ESP_LOGI(TAG, "--> received check sensor status command");

    trigger_heartbeat_now();
}

// ==================== Helper Functions ====================

static bool topic_matches(const char *topic, size_t topic_len, const char *expected_topic)
{
    if (!topic || !expected_topic)
        return false;

    size_t expected_len = strlen(expected_topic);

    return (topic_len == expected_len) && (strncmp(topic, expected_topic, topic_len) == 0);
}

static bool copy_payload(char *dst, size_t dst_size, const char *data, int d_len)
{
    if (!dst || !data || d_len <= 0 || (size_t)d_len >= dst_size)
        return false;

    memcpy(dst, data, (size_t)d_len);
    dst[d_len] = '\0';
    return true;
}

static bool parse_u32_decimal(const char *text, uint32_t *value_out)
{
    if (!text || !value_out || *text == '\0')
        return false;

    uint32_t value = 0;
    for (const char *p = text; *p != '\0'; ++p)
    {
        if (*p < '0' || *p > '9')
            return false;

        uint32_t digit = (uint32_t)(*p - '0');
        if (value > (UINT32_MAX - digit) / 10U)
            return false;

        value = value * 10U + digit;
    }

    *value_out = value;
    return true;
}

static bool extract_json_rate_value(const char *json,
                                    const char *key,
                                    uint32_t *value_out)
{
    if (!json || !key || !value_out)
        return false;

    char key_pattern[64];
    int key_len = snprintf(key_pattern, sizeof(key_pattern), "\"%s\"", key);
    if (key_len <= 0 || (size_t)key_len >= sizeof(key_pattern))
        return false;

    const char *key_pos = strstr(json, key_pattern);
    if (!key_pos)
        return false;

    const char *value_pos = strchr(key_pos + key_len, ':');
    if (!value_pos)
        return false;

    ++value_pos;
    while (*value_pos == ' ' || *value_pos == '\t' || *value_pos == '\n' || *value_pos == '\r')
        ++value_pos;

    bool quoted = (*value_pos == '"');
    if (quoted)
        ++value_pos;

    char number_buf[16];
    size_t i = 0;
    while (*value_pos >= '0' && *value_pos <= '9')
    {
        if (i >= sizeof(number_buf) - 1)
            return false;
        number_buf[i++] = *value_pos++;
    }

    if (i == 0)
        return false;

    number_buf[i] = '\0';

    if (quoted && *value_pos != '"')
        return false;

    return parse_u32_decimal(number_buf, value_out);
}
