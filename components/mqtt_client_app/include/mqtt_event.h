#pragma once

#include <stdint.h>

#include "esp_event_base.h"
#include "mqtt_types.h"

void mqtt_app_event_handler(void *handler_args,
                            esp_event_base_t base,
                            int32_t event_id,
                            void *event_data);
