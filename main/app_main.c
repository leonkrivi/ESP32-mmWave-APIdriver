#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "mr24hpc.h"
#include "wifi_manager.h"
#include "mqtt_app.h"

static const char *TAG = "* app_main *";

// called from
// mr24hpc.c : mr24hpc_update_state(...)
// when new state is available
void my_state_callback(const mr24hpc_state_t *state) {
    // TODO: napisi callback funkciju koja obrađuje novo stanje senzora
    return;
}

void app_main(void) {

    printf("================  start of APP output ================\n");
    mr24hpc_init();
    mr24hpc_register_callback(my_state_callback);
    mr24hpc_start();

    ESP_LOGW(TAG, "Initializing WiFi...");
    wifi_manager_init();
    ESP_LOGW(TAG, "Waiting for WiFi connection...");
    wifi_manager_wait_connected();

    mqtt_app_start();

    ESP_LOGW(TAG, "App main is running");
    // while (1) {
    //     printf(". ");
    //     fflush(stdout);
    //     vTaskDelay(pdMS_TO_TICKS(2000));
    // }

    // mr24hpc_state_t state;
    // while(1) {
    //     if(mr24hpc_get_state(&state)) {
    //         printf("==========================================================\n");
    //         ESP_LOGI(TAG, "Successfully retrieved sensor state:\n");
    //         printf("----------------------------------------------------------\n");
    //         printf("Presence: %s\n", state.presence ? "Yes" : "No");
    //         printf("Motion State: %u\n", state.motion_state);
    //         printf("Distance (m): %.2f\n", state.distance_m);
    //         printf("Speed (m/s): %.2f\n", state.speed_m_s);
    //         printf("Body Signals: %u\n", state.body_signals);
    //         printf("Last Update (ms): %lu\n", state.last_update_ms);
    //         printf("==========================================================\n");
    //     }
    //     vTaskDelay(pdMS_TO_TICKS(1000));
    // }
}