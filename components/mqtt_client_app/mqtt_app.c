#include <stdio.h>

#include "esp_log.h"
#include "mqtt_client.h"

#include "mqtt_app.h"

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

ESP_EVENT_DEFINE_BASE(MQTT_APP_USER_EVENT_BASE);

static const char *TAG ="* mqtt_app *";

static esp_mqtt_client_handle_t mqtt_client = NULL;
static esp_mqtt_client_config_t mqtt_broker_cfg =
    {
        .broker.address.uri = "mqtt://" MQTT_BROKER_IP ":1883" // From .env file, loaded with CMake
    };
static int retry_num = 0;


static void user_event_handler(void *handler_args,
                               esp_event_base_t event_base,
                               int32_t event_id,
                               void *event_data)
{
    if (event_id == MQTT_APP_USER_EVENT_STOP && mqtt_client) {
        esp_mqtt_client_stop(mqtt_client);
        esp_mqtt_client_destroy(mqtt_client);
        mqtt_client = NULL;
    }
}


static void event_handler(void *handler_args,
                          esp_event_base_t base,
                          int32_t event_id,
                          void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;

    switch (event_id) {
        case MQTT_EVENT_BEFORE_CONNECT:
            if (retry_num < 1)
                ESP_LOGW(TAG, "Client initialized, connecting...");
            break;
        case MQTT_EVENT_CONNECTED:
            retry_num = 0;
            ESP_LOGW(TAG, "Client connected to the broker: %s", mqtt_broker_cfg.broker.address.uri);
            ESP_LOGW(TAG, "Subscribing to topic /esp32/test");
            msg_id = esp_mqtt_client_subscribe_single(client, "/esp32/test", 0);
            ESP_LOGW(TAG, "Subscription request sent, msg_id=%d", msg_id);
            ESP_LOGW(TAG, "Publishing to topic /esp32/test");
            msg_id = esp_mqtt_client_publish(client, "/esp32/test", "--> TEST MESSAGE <--", 0, 1, 0);
            ESP_LOGW(TAG, "Message published, msg_id=%d", msg_id);
            break;
        case MQTT_EVENT_DISCONNECTED:
            if (retry_num < 3) {
                retry_num++;
                uint32_t delay_ms = retry_num * 3000;
                ESP_LOGW(TAG, "Client disconnected, reconnecting...");
                vTaskDelay(pdMS_TO_TICKS(delay_ms));
                esp_mqtt_client_reconnect(client);
            } 
            else {
                retry_num++;
                ESP_LOGE(TAG, "Client disconnected, timeout reached");
                ESP_LOGW(TAG, "Stopping MQTT client");
                ESP_ERROR_CHECK(esp_event_post(MQTT_APP_USER_EVENT_BASE, MQTT_APP_USER_EVENT_STOP, NULL, 0, portMAX_DELAY));
            }
            break;
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGW(TAG, "Subscribed successfully (check: msg_id=%d)", event->msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGW(TAG, "Unsubscribed successfully (check: msg_id=%d)", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGW(TAG, "Message published successfully (check: msg_id=%d)", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGW(TAG, "Message received on topic: %.*s", event->topic_len, event->topic);
            ESP_LOGW(TAG, "| -- data: %.*s", event->data_len, event->data);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT event error occurred");
            ESP_LOGE(TAG, "Error type: %d", event->error_handle->error_type);
            break;
        default:
            ESP_LOGE(TAG, "Unknown event id:%d", event_id);
            break;
    }
}


void mqtt_app_start(void) {

    ESP_LOGW(TAG, "Connecting to broker: %s", mqtt_broker_cfg.broker.address.uri); 

    mqtt_client = esp_mqtt_client_init(&mqtt_broker_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, event_handler, NULL);
    esp_event_handler_register(MQTT_APP_USER_EVENT_BASE, ESP_EVENT_ANY_ID, user_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);

    return;
}

void mqtt_app_publish_state(const mr24hpc_state_t *state) {};