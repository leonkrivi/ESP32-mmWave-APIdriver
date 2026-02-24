#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "esp_err.h"

esp_err_t wifi_manager_init(void);
void wifi_manager_wait_connected(TickType_t timeout_ticks);
bool wifi_manager_is_connected(void);