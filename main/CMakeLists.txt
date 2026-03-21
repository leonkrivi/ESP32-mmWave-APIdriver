
idf_component_register(SRCS "app_main.c" "sensor_state_tracker.c" "sensor_logger.c"
                       REQUIRES mr24hpc wifi_manager mqtt_client_app)
