#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"

#include "mr24hpc.h"
#include "mr24hpc_types_uof.h"
#include "mqtt_types.h"

#define MQTT_TOPIC_ROOM_STATE "/room/" ROOM_ID "/device/" DEVICE_ID "/room_state"
#define MQTT_TOPIC_CONFIGURATION "/room/" ROOM_ID "/device/" DEVICE_ID "/configuration"
#define MQTT_TOPIC_CONNECTION_STATUS "/room/" ROOM_ID "/device/" DEVICE_ID "/connection/status"     // LWT topic, esp publishes "online" or "offline" retained messages to this topic
#define MQTT_TOPIC_SENSOR_STATUS "/room/" ROOM_ID "/device/" DEVICE_ID "/sensor/status"             // esp is publisher to this topic
                                                                                                    // indicates connection status and heartbeat interval
#define MQTT_TOPIC_SENSOR_STATUS_CHECK "/room/" ROOM_ID "/device/" DEVICE_ID "/sensor/status/check" // esp is subscriber to this topic
                                                                                                    // receives "check connection" command to trigger heartbeat immediately
void mqtt_app_start(const char *device_id,
                    const char *room_id,
                    const char *connection_status_topic,
                    const char *configuration_topic,
                    const char *sensor_status_topic,
                    const char *sensor_status_check_topic);

void mqtt_app_register_rate_callbacks(mqtt_hb_rate_change_cb_t hb_cb,
                                      mqtt_sensor_rate_change_cb_t sensor_cb);

void mqtt_app_publish_sensor_status(const char *topic, const char *status, uint32_t hb_interval);
void mqtt_app_publish_state(const char *topic, const mr24hpc_state_t *state);
void mqtt_app_publish_uof_state(const char *topic, const UOF_mr24hpc_state_t *state);

bool mqtt_app_is_connected(void);
void mqtt_app_wait_connected(TickType_t timeout_ticks);