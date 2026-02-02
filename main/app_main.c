#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "mr24hpc.h"
#include "wifi_manager.h"
#include "mqtt_app.h"

static const char *TAG = "* app_main *";

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
void my_state_callback(const mr24hpc_state_t *state) {
    printf("\n========== MR24HPC Sensor Update ==========\n");
    
    // Show which fields are valid in this update
    // printf("Valid fields: ");
    // if (state->valid_mask & MR24HPC_VALID_EXISTENCE_ENERGY) printf("ExistEnergy ");
    // if (state->valid_mask & MR24HPC_VALID_STATIC_DISTANCE)  printf("StaticDist ");
    // if (state->valid_mask & MR24HPC_VALID_MOTION_ENERGY)    printf("MotionEnergy ");
    // if (state->valid_mask & MR24HPC_VALID_MOTION_DISTANCE)  printf("MotionDist ");
    // if (state->valid_mask & MR24HPC_VALID_MOTION_SPEED)     printf("Speed ");
    // if (state->valid_mask & MR24HPC_VALID_DIRECTION)        printf("Direction ");
    // if (state->valid_mask & MR24HPC_VALID_MOVING_PARAMS)    printf("MovingParams ");
    // printf("\n--------------------------------------------\n");

    // Existence / static presence
    printf("Existence Energy:   %3u / 250\n", state->existence_energy);
    printf("Static Distance:    %.1f m\n", state->static_distance_m);

    // Motion
    printf("Motion Energy:      %3u / 250\n", state->motion_energy);
    printf("Motion Distance:    %.1f m\n", state->motion_distance_m);
    printf("Motion Speed:       %+.1f m/s\n", state->motion_speed_m_s);
    printf("Direction:          %s\n", direction_to_str(state->direction));

    // Other
    printf("Moving Params:      %u / 100\n", state->moving_params);
    printf("Last Update:        %lu ms ago\n", state->last_update_ms);
    printf("============================================\n\n");
}

void app_main(void) {

    printf("================  start of APP output ================\n");
    mr24hpc_init();
    mr24hpc_register_callback(my_state_callback);
    mr24hpc_start();

    // ESP_LOGW(TAG, "Initializing WiFi...");
    // wifi_manager_init();
    // ESP_LOGW(TAG, "Waiting for WiFi connection...");
    // wifi_manager_wait_connected(20000);

    // mqtt_app_start();

    ESP_LOGW(TAG, "App main is running");

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }

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