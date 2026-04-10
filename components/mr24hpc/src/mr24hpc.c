#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_err.h"

#include "mr24hpc.h"
#include "mr24hpc_types_uof.h"
#include "mr24hpc_parser.h"
#include "mr24hpc_state_update.h"
#include "mr24hpc_uart.h"

static QueueHandle_t uart_rx_queue = NULL;
SemaphoreHandle_t state_mutex = NULL;

static mr24hpc_parser_config_t g_parser_cfg;
static bool g_uof_mode_enabled = false;

mr24hpc_state_t g_sensor_state;
UOF_mr24hpc_state_t g_uof_sensor_state;

state_callback g_state_callback = NULL;
heartbeat_callback g_heartbeat_callback = NULL;

static uint32_t g_query_interval_ms = 5000;
static uint32_t g_hearbeat_interval_ms = 30000;
static const uint8_t g_standard_query_cmds[] = {0x81, 0x82, 0x83, 0x8B};
static const uint8_t g_uof_query_cmds[] = {0x01, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87};
static const uint8_t g_heartbeat_packet[] = {0x53, 0x59, 0x01, 0x01, 0x00, 0x01, 0x0F, 0xBE, 0x54, 0x43};

// ==================== Forward Declarations ====================

static void uart_task(void *arg);
static void driver_task(void *arg);
static void query_task(void *arg);
static void heartbeatTask(void *arg);

size_t build_query_packet(uint8_t ctrl, uint8_t cmd, uint8_t *out_packet, size_t out_size);
static const uint8_t *active_query_cmds(int *count);
static uint8_t calculate_checksum(const uint8_t *data, size_t len);

// ==================== Personal Header functions ====================

esp_err_t mr24hpc_init(bool UOF)
{
    g_uof_mode_enabled = UOF;

    uart_init();
    uart_rx_queue = xQueueCreate(256, sizeof(uint8_t));

    parser_init(&g_parser_cfg, UOF);

    state_mutex = xSemaphoreCreateMutex();

    return ESP_OK;
}

esp_err_t mr24hpc_start(void)
{
    xTaskCreate(uart_task, "mr24_uart", 2048, NULL, 10, NULL);
    xTaskCreate(driver_task, "mr24_drv", 4096, NULL, 9, NULL);
    xTaskCreate(query_task, "mr24_qry", 3072, NULL, 8, NULL);
    xTaskCreate(heartbeatTask, "mr24_hb", 2048, NULL, 5, NULL);
    return ESP_OK;
}

esp_err_t register_state_callback(state_callback cb)
{
    // if (!state_mutex) return ESP_FAIL;
    state_lock();
    g_state_callback = cb;
    state_unlock();
    return ESP_OK;
}

esp_err_t register_heartbeat_callback(heartbeat_callback cb)
{
    state_lock();
    g_heartbeat_callback = cb;
    state_unlock();
    return ESP_OK;
}

bool mr24hpc_get_state(mr24hpc_state_t *state_copy)
{
    if (!state_copy)
        return false;

    state_lock();
    *state_copy = g_sensor_state;
    state_unlock();

    return true;
}

bool mr24hpc_get_uof_state(UOF_mr24hpc_state_t *state_copy)
{
    if (!state_copy)
        return false;

    state_lock();
    *state_copy = g_uof_sensor_state;
    state_unlock();

    return true;
}

bool mr24hpc_is_uof_mode(void)
{
    return g_uof_mode_enabled;
}

esp_err_t trigger_heartbeat_now(void)
{
    uart_write(g_heartbeat_packet, sizeof(g_heartbeat_packet));
    return ESP_OK;
}

esp_err_t set_heartbeat_interval_ms(uint32_t interval_ms)
{
    state_lock();
    g_hearbeat_interval_ms = interval_ms;
    state_unlock();

    return ESP_OK;
}

uint32_t get_heartbeat_interval_ms(void)
{
    uint32_t interval_ms;

    state_lock();
    interval_ms = g_hearbeat_interval_ms;
    state_unlock();

    return interval_ms;
}

esp_err_t set_sensor_rate_interval_ms(uint32_t interval_ms)
{
    state_lock();
    g_query_interval_ms = interval_ms;
    state_unlock();

    return ESP_OK;
}

uint32_t get_sensor_rate_interval_ms(void)
{
    uint32_t interval_ms;

    state_lock();
    interval_ms = g_query_interval_ms;
    state_unlock();

    return interval_ms;
}

// ==================== Local functions ====================

// ========== Tasks ==========

static void uart_task(void *arg)
{
    uint8_t byte;
    while (1)
    {
        if (uart_read(&byte, 1, 100) == 1)
        {
            xQueueSend(uart_rx_queue, &byte, portMAX_DELAY);
        }
    }
}

static void driver_task(void *arg)
{
    uint8_t byte;
    while (1)
    {
        if (xQueueReceive(uart_rx_queue, &byte, portMAX_DELAY))
        {
            parser_feed(&g_parser_cfg, byte);
        }
    }
}

static void query_task(void *arg)
{
    uint8_t packet[10];

    while (1)
    {
        uint32_t interval_ms = get_sensor_rate_interval_ms();

        int command_count = 0;
        uint8_t control = g_uof_mode_enabled ? 0x08 : 0x80;
        const uint8_t *commands = active_query_cmds(&command_count);

        for (int i = 0; i < command_count; ++i)
        {
            size_t packet_len = build_query_packet(control, commands[i], packet, sizeof(packet));
            if (packet_len > 0)
            {
                uart_write(packet, packet_len);
            }
            vTaskDelay(pdMS_TO_TICKS(30));
        }

        vTaskDelay(pdMS_TO_TICKS(interval_ms));
    }
}

static void heartbeatTask(void *arg)
{
    while (1)
    {
        uint32_t interval_ms = get_heartbeat_interval_ms();

        uart_write(g_heartbeat_packet, sizeof(g_heartbeat_packet));
        vTaskDelay(interval_ms / portTICK_PERIOD_MS); // 30s
    }
}

// ========== Helpers ==========

size_t build_query_packet(uint8_t ctrl, uint8_t cmd, uint8_t *out_packet, size_t out_size)
{
    if (!out_packet || out_size < 10)
        return 0;

    out_packet[0] = MR24_FRAME_HEADER_1;
    out_packet[1] = MR24_FRAME_HEADER_2;
    out_packet[2] = ctrl;
    out_packet[3] = cmd;
    out_packet[4] = 0x00;
    out_packet[5] = 0x01;
    out_packet[6] = 0x0F;
    out_packet[7] = calculate_checksum(out_packet, 7);
    out_packet[8] = MR24_FRAME_END_1;
    out_packet[9] = MR24_FRAME_END_2;

    return 10;
}

static const uint8_t *active_query_cmds(int *count)
{
    if (!count)
        return NULL;

    if (g_uof_mode_enabled)
    {
        *count = sizeof(g_uof_query_cmds);
        return g_uof_query_cmds;
    }

    *count = sizeof(g_standard_query_cmds);
    return g_standard_query_cmds;
}

static uint8_t calculate_checksum(const uint8_t *data, size_t len)
{
    uint32_t sum = 0;

    for (size_t i = 0; i < len; ++i)
    {
        sum += data[i];
    }

    return (uint8_t)(sum & 0xFF);
}
