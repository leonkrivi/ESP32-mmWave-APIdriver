#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "wifi_manager.h"

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

const char *wifi_ssid = STR(WIFI_SSID);         // From .env file, loaded with CMake
const char *wifi_password = STR(WIFI_PASSWORD); // as compiler definitions

#define WIFI_CONNECTED_BIT (1 << 0)
#define WIFI_FAIL_BIT      (1 << 1)

static const char *TAG = "wifi_manager";
static int retry_num = 0;

static EventGroupHandle_t wifi_event_group;
esp_netif_t *netif_instance;
static bool wifi_connected = false;

static void wifi_event_handler(void *handler_args,
                               esp_event_base_t event_base,
                               int32_t event_id,
                               void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    }

    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (retry_num < 5) {
            esp_wifi_connect();
            retry_num++;
            ESP_LOGW(TAG, "WiFi disconnected, retrying...");
        } else {
            ESP_LOGE(TAG, "WiFi disconnected, timeout reached");
            xEventGroupSetBits(wifi_event_group, WIFI_FAIL_BIT);
        }
        wifi_connected = false;
    }

    // IP je dodijeljen kasnije, zato se koristi eventGroup za sinkronizaciju
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        wifi_connected = true;
        retry_num = 0;
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);   // Signal that we are connected
        ESP_LOGI(TAG, "WiFi connected, IP acquired");
    }
}

esp_err_t wifi_manager_init(void)
{
    esp_err_t ret;
    
    wifi_event_group = xEventGroupCreate();
    // if (!wifi_event_group) {
    //     ESP_LOGE(TAG, "Failed to create WiFi event group");
    //     return ESP_FAIL;
    // }

    // WiFi needs NVS (Non-Volatile Storage) to store settings
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    netif_instance = esp_netif_create_default_wifi_sta();

    wifi_init_config_t init_wifi_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&init_wifi_config));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi init done");

    return ESP_OK;
}

void wifi_manager_wait_connected(void)
{
    xEventGroupWaitBits(
        wifi_event_group,
        WIFI_CONNECTED_BIT,
        pdFALSE,
        pdTRUE,
        portMAX_DELAY);
}

bool wifi_manager_is_connected(void)
{
    return wifi_connected;
}
