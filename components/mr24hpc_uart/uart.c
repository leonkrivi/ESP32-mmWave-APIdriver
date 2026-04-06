#include "driver/uart.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "mr24hpc_uart.h"

#define MR24HPC_UART UART_NUM_1
#define MR24HPC_UART_TX GPIO_NUM_17
#define MR24HPC_UART_RX GPIO_NUM_16
#define MR24HPC_BAUDRATE 115200

// ==================== Personal Header functions ====================

void uart_init(void)
{
    const uart_config_t cfg = {
        .baud_rate = MR24HPC_BAUDRATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE};

    uart_driver_install(MR24HPC_UART, 2048, 0, 0, NULL, 0);
    uart_param_config(MR24HPC_UART, &cfg);
    uart_set_pin(MR24HPC_UART, MR24HPC_UART_TX, MR24HPC_UART_RX,
                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

int uart_read(uint8_t *buf, size_t len, uint32_t timeout_ms)
{
    return uart_read_bytes( // thread-safe internally (doesnt need external mutex)
        MR24HPC_UART,
        buf,
        len,
        pdMS_TO_TICKS(timeout_ms));
}

int uart_write(const uint8_t *buf, size_t len)
{
    return uart_write_bytes(MR24HPC_UART, (const char *)buf, len);
}