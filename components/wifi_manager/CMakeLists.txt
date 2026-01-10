
idf_component_register(SRCS "wifi_manager.c"
                       REQUIRES "esp_wifi" "esp_netif" "nvs_flash"
                       INCLUDE_DIRS "include")
