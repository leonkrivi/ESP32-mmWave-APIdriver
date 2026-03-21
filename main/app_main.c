#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "mr24hpc.h"
#include "wifi_manager.h"
#include "mqtt_app.h"
#include "sensor_state_tracker.h"
#include "sensor_logger.h"

static const char *TAG = "* app_main *";

static uint32_t g_frequency = 5000; // default: 5 seconds

// MQTT topics controlled from app layer
#define MQTT_TOPIC_STATE "/test/backend"
#define MQTT_TOPIC_UOF_STATE "/test/backend"
#define MQTT_TOPIC_RATE_COMMAND "/test/esp32"

// ==================== Forward Declarations ====================

static void on_state_change(const mr24hpc_state_t *state);
static void on_rate_change(uint32_t interval_ms);

// ==================== Main Application ====================
void app_main(void)
{
    bool uof_enabled = false;
    sensor_state_tracker_init();

    printf("================  start of APP output ================\n");
    ESP_LOGW(TAG, "Initializing MR24HPC sensor...");
    mr24hpc_init(uof_enabled);
    mr24hpc_set_query_interval_ms(g_frequency);
    mr24hpc_register_callback(on_state_change);
    mr24hpc_start();

    ESP_LOGW(TAG, "Initializing WiFi...");
    wifi_manager_init();
    ESP_LOGW(TAG, "Waiting for WiFi connection...");
    wifi_manager_wait_connected(portMAX_DELAY);

    ESP_LOGW(TAG, "Initializing MQTT...");
    mqtt_app_register_rate_callback(on_rate_change);
    mqtt_app_start(MQTT_TOPIC_RATE_COMMAND);
    ESP_LOGW(TAG, "Waiting for MQTT connection...");
    mqtt_app_wait_connected(portMAX_DELAY);

    ESP_LOGW(TAG, "App main is running");

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

// ==================== Callbacks ====================

static void on_rate_change(uint32_t interval_ms)
{
    g_frequency = interval_ms;
    mr24hpc_set_query_interval_ms(interval_ms);
    ESP_LOGW(TAG, "Frequency changed to %lu ms", interval_ms);
}

// MR24HPC state change callback
static void on_state_change(const mr24hpc_state_t *state)
{
    UOF_mr24hpc_state_t uof_state = {0};
    sensor_tracker_result_t tracker_result;

    // important when recconnecting to MQTT
    if (!mqtt_app_is_connected())
        return;

    if (mr24hpc_is_uof_mode())
    {
        if (!mr24hpc_get_uof_state(&uof_state))
            return;

        tracker_result = sensor_state_tracker_evaluate_uof(&uof_state);
        if (tracker_result == SENSOR_TRACKER_NO_CHANGE)
            return;

        if (tracker_result == SENSOR_TRACKER_CHANGED)
            sensor_logger_print_uof(&uof_state, g_frequency);

        mqtt_app_publish_uof_state(MQTT_TOPIC_UOF_STATE, &uof_state);
        return;
    }

    tracker_result = sensor_state_tracker_evaluate(state);
    if (tracker_result == SENSOR_TRACKER_NO_CHANGE)
        return;

    if (tracker_result == SENSOR_TRACKER_CHANGED)
        // console output (for debugging)
        sensor_logger_print_standard(state, g_frequency);

    // send via MQTT if state changed or first sample
    mqtt_app_publish_state(MQTT_TOPIC_STATE, state);
}