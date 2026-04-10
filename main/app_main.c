#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "mr24hpc.h"
#include "wifi_manager.h"
#include "mqtt_app.h"
#include "sensor_state_tracker.h"
#include "sensor_logger.h"

#define INITIAL_SENSOR_RATE_INTERVAL_MS 5000
#define INITIAL_HEARTBEAT_RATE_INTERVAL_MS 10000

#define DEVICE_ID "01"
#define ROOM_NAME "living_room"

static const char *TAG = "* app_main *";

// ==================== Forward Declarations ====================

static void on_state_change(const mr24hpc_state_t *state);
static void on_hb_rate_change(uint32_t interval_ms);
static void on_sensor_rate_change(uint32_t interval_ms);
static void on_heartbeat_detected(void);

// ==================== Main Application ====================
void app_main(void)
{
    bool uof = false;
    sensor_state_tracker_init();

    printf("================  start of APP output ================\n");
    ESP_LOGW(TAG, "Initializing MR24HPC sensor...");
    mr24hpc_init(uof);

    set_sensor_rate_interval_ms(INITIAL_SENSOR_RATE_INTERVAL_MS);  // set initial frequency before callbacks start coming in
    set_heartbeat_interval_ms(INITIAL_HEARTBEAT_RATE_INTERVAL_MS); // set heartbeat interval to 10s

    register_state_callback(on_state_change);
    register_heartbeat_callback(on_heartbeat_detected);
    mr24hpc_start();

    ESP_LOGW(TAG, "Initializing WiFi...");
    wifi_manager_init();
    ESP_LOGW(TAG, "Waiting for WiFi connection...");
    wifi_manager_wait_connected(portMAX_DELAY);

    ESP_LOGW(TAG, "Initializing MQTT...");
    mqtt_app_register_rate_callbacks(on_hb_rate_change, on_sensor_rate_change);
    mqtt_app_start(DEVICE_ID, ROOM_NAME, MQTT_TOPIC_CONNECTION_STATUS, MQTT_TOPIC_CONFIGURATION, MQTT_TOPIC_SENSOR_STATUS, MQTT_TOPIC_SENSOR_STATUS_CHECK);
    ESP_LOGW(TAG, "Waiting for MQTT connection...");
    mqtt_app_wait_connected(portMAX_DELAY);

    ESP_LOGW(TAG, "App main is running");

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

// ==================== Callbacks ====================

static void on_state_change(const mr24hpc_state_t *state)
{
    UOF_mr24hpc_state_t uof_state = {0};
    sensor_tracker_result_t tracker_result;

    // important when recconnecting to MQTT
    if (!mqtt_app_is_connected())
        return;

    // uof mode state evaluation
    if (mr24hpc_is_uof_mode())
    {
        if (!mr24hpc_get_uof_state(&uof_state))
            return;

        tracker_result = sensor_state_tracker_evaluate_uof(&uof_state);
        if (tracker_result == SENSOR_TRACKER_CYCLE_INCOMPLETE || tracker_result == SENSOR_TRACKER_NO_CHANGE)
            return;

        // for logging and debugging
        // if (tracker_result == SENSOR_TRACKER_CHANGED || tracker_result == SENSOR_TRACKER_FIRST_SAMPLE)
        //     sensor_logger_print_uof(&uof_state, g_frequency);

        mqtt_app_publish_uof_state(MQTT_TOPIC_ROOM_STATE, &uof_state);
        return;
    }

    // standard mode state evaluation
    tracker_result = sensor_state_tracker_evaluate(state);
    if (tracker_result == SENSOR_TRACKER_CYCLE_INCOMPLETE || tracker_result == SENSOR_TRACKER_NO_CHANGE)
        return;

    // for logging and debugging
    // if (tracker_result == SENSOR_TRACKER_CHANGED || tracker_result == SENSOR_TRACKER_FIRST_SAMPLE)
    //     sensor_logger_print_standard(state, g_frequency);

    // send via MQTT if state changed or first sample
    mqtt_app_publish_state(MQTT_TOPIC_ROOM_STATE, state);
}

static void on_hb_rate_change(uint32_t interval_ms)
{
    set_heartbeat_interval_ms(interval_ms);
    ESP_LOGW(TAG, "Heartbeat interval changed to %lu ms", interval_ms);
}

static void on_sensor_rate_change(uint32_t interval_ms)
{
    set_sensor_rate_interval_ms(interval_ms);
    ESP_LOGW(TAG, "Sensor rate changed to %lu ms", interval_ms);
}

static void on_heartbeat_detected(void)
{
    ESP_LOGW(TAG, "Heartbeat detected");
    mqtt_app_publish_sensor_status(MQTT_TOPIC_SENSOR_STATUS, "alive", get_heartbeat_interval_ms());
}