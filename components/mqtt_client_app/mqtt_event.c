#include <stdbool.h>
#include <inttypes.h>
#include <string.h>

#include "esp_log.h"

#include "mqtt_event.h"

static const char *TAG = "* mqtt_app *";

// ==================== Forward Declarations ====================
static bool topic_matches(const char *topic, int t_len, const char *expected_topic);
static void process_rate_command_message(mqtt_app_context_t *ctx,
                                         const char *topic,
                                         int t_len,
                                         const char *data,
                                         int d_len);
static void process_connection_status_message(mqtt_app_context_t *ctx,
                                              const char *topic,
                                              int t_len,
                                              const char *data,
                                              int d_len);
static bool copy_payload(char *dst, size_t dst_size, const char *data, int d_len);
static bool parse_u32_decimal(const char *text, uint32_t *value_out);

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
        ESP_LOGW(TAG, "Client connected to broker: %s", ctx->broker_cfg.broker.address.uri);
        if (ctx->rate_command_topic)
            esp_mqtt_client_subscribe_single(client, ctx->rate_command_topic, 1);
        if (ctx->connection_status_topic)
            esp_mqtt_client_subscribe_single(client, ctx->connection_status_topic, 1);
        break;

    case MQTT_EVENT_DISCONNECTED:
        if (ctx->event_group)
            xEventGroupClearBits(ctx->event_group, MQTT_CONNECTED_BIT);
        ESP_LOGW(TAG, "Disconnected, auto-reconnect in %d s...", MQTT_RECONNECT_DELAY_MS / 1000);
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

        process_rate_command_message(ctx, topic, t_len, data, d_len);
        process_connection_status_message(ctx, topic, t_len, data, d_len);
        break;
    }

    default:
        break;
    }
}

// ==================== Helper Functions ====================
static bool topic_matches(const char *topic, int t_len, const char *expected_topic)
{
    if (!topic || t_len < 0 || !expected_topic)
        return false;

    size_t expected_len = strlen(expected_topic);
    return ((size_t)t_len == expected_len) && (strncmp(topic, expected_topic, expected_len) == 0);
}

static bool copy_payload(char *dst, size_t dst_size, const char *data, int d_len)
{
    if (!dst || !data || d_len <= 0 || (size_t)d_len >= dst_size || d_len > MQTT_MAX_CMD_PAYLOAD_LEN)
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

static void process_rate_command_message(mqtt_app_context_t *ctx,
                                         const char *topic,
                                         int topic_len,
                                         const char *data,
                                         int data_len)
{
    if (!ctx || !topic_matches(topic, topic_len, ctx->rate_command_topic))
        return;

    char payload[MQTT_MAX_CMD_PAYLOAD_LEN + 1];
    if (!copy_payload(payload, sizeof(payload), data, data_len))
        return;

    uint32_t interval_ms = 0;
    if (!parse_u32_decimal(payload, &interval_ms) || interval_ms == 0)
        return;

    if (!ctx->rate_change_cb)
        return;

    ESP_LOGI(TAG, "--> received rate change command: %" PRIu32 " ms", interval_ms);

    ctx->rate_change_cb(interval_ms); // call the registered callback to change the rate
}

static void process_connection_status_message(mqtt_app_context_t *ctx,
                                              const char *topic,
                                              int topic_len,
                                              const char *data,
                                              int data_len)
{
    if (!ctx || !topic_matches(topic, topic_len, ctx->connection_status_topic))
        return;

    char payload[MQTT_MAX_CMD_PAYLOAD_LEN + 1];
    if (!copy_payload(payload, sizeof(payload), data, data_len) || strcmp(payload, "check connection") != 0)
        return;

    if (!ctx->connection_check_cb)
        return;

    ESP_LOGI(TAG, "--> received check connection status command");

    ctx->connection_check_cb(); // call the registered callback to check the connection status
}
