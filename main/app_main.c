#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "mr24hpc.h"
#include "wifi_manager.h"
#include "mqtt_app.h"

static const char *TAG = "* app_main *";

// ==================== Primitive Rate Limiting ====================
static uint32_t g_send_interval_ms = 5000;  // default: 5 seconds
static uint32_t g_last_send_ms = 0;

static uint32_t get_time_ms(void) {
    return (uint32_t)(esp_timer_get_time() / 1000ULL);
}

// ==================== MQTT rate limiting ====================
// MQTT rate change callback
void on_rate_change(uint32_t interval_ms) {
    g_send_interval_ms = interval_ms;
    ESP_LOGW(TAG, "Send interval changed to %lu ms", interval_ms);
}

// Helper to convert direction enum to string
static const char* direction_to_str(mr24hpc_direction_t dir) {
    switch (dir) {
        case MR24HPC_DIR_APPROACHING: return "APPROACHING";
        case MR24HPC_DIR_MOVING_AWAY: return "MOVING AWAY";
        default: return "NONE/STATIONARY";
    }
}

// called from
// mr24hpc.c : mr24hpc_update_state(...)
// when new state is available
void on_state_change(const mr24hpc_state_t *state) {
    // Block until MQTT is connected
    if (!mqtt_app_is_connected()) {
        return;
    }

    uint32_t now = get_time_ms();
    
    // primitive rate limiting
    if (g_send_interval_ms > 0 && (now - g_last_send_ms) < g_send_interval_ms) {
        return; // skip
    }
    g_last_send_ms = now;

    // Console output (for debugging)
    printf("\n========== MR24HPC Sensor Update ==========\n");
    printf("(interval: %lu ms)\n", g_send_interval_ms);
    printf("Existence Energy:   %3u / 250\n", state->existence_energy);
    printf("Static Distance:    %.1f m\n", state->static_distance_m);
    printf("Motion Energy:      %3u / 250\n", state->motion_energy);
    printf("Motion Distance:    %.1f m\n", state->motion_distance_m);
    printf("Motion Speed:       %+.1f m/s\n", state->motion_speed_m_s);
    printf("Direction:          %s\n", direction_to_str(state->direction));
    printf("Moving Params:      %u / 100\n", state->moving_params);
    printf("============================================\n\n");

    // send via MQTT
    mqtt_app_publish_state(state);
}

void app_main(void) {

    printf("================  start of APP output ================\n");
    ESP_LOGW(TAG, "Initializing MR24HPC sensor...");
    mr24hpc_init();
    mr24hpc_register_callback(on_state_change);
    mr24hpc_start();

    ESP_LOGW(TAG, "Initializing WiFi...");
    wifi_manager_init();
    ESP_LOGW(TAG, "Waiting for WiFi connection...");
    wifi_manager_wait_connected(20000);

    ESP_LOGW(TAG, "Initializing MQTT...");
    mqtt_app_register_rate_callback(on_rate_change);
    mqtt_app_start();

    ESP_LOGW(TAG, "App main is running");

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }

}