cmake_minimum_required(VERSION 3.16)

# -----------------------------
# Load .env file (developer local)
# -----------------------------
set(ENV_FILE "${CMAKE_SOURCE_DIR}/.env")

# Re-run CMake configure whenever .env changes.
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${ENV_FILE}")

if(EXISTS ${ENV_FILE})
    file(STRINGS ${ENV_FILE} ENV_LINES)
    foreach(line ${ENV_LINES})
        if(line MATCHES "^([^#][^=]+)=(.+)$")
            set(ENV{${CMAKE_MATCH_1}} ${CMAKE_MATCH_2})
        endif()
    endforeach()
else()
    message(WARNING ".env file not found, WiFi credentials may be missing")
endif()

if(DEFINED ENV{WIFI_SSID} AND DEFINED ENV{WIFI_PASSWORD} AND DEFINED ENV{MQTT_BROKER_IP})
    add_compile_definitions(
        WIFI_SSID="$ENV{WIFI_SSID}"
        WIFI_PASSWORD="$ENV{WIFI_PASSWORD}"
        MQTT_BROKER_IP="$ENV{MQTT_BROKER_IP}"
    )
else()
    message(FATAL_ERROR "WIFI_SSID or WIFI_PASSWORD or MQTT_BROKER_IP not set. Check your .env file.")
endif()

# -----------------------------
# ESP-IDF boilerplate (comes after .env loading)
# -----------------------------

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
# "Trim" the build. Include the minimal set of components, main, and anything it depends on.
idf_build_set_property(MINIMAL_BUILD ON)
project(ESP_mmWave_APIdriver)
