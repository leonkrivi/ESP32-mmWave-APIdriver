#pragma once

#include <stddef.h>
#include <stdint.h>

void mr24hpc_uart_init(void);
int  mr24hpc_uart_read(uint8_t *buf, size_t len, uint32_t timeout_ms);
int  mr24hpc_uart_write(const uint8_t *buf, size_t len);

// void mr24hpc_uart_lock(void);
// void mr24hpc_uart_unlock(void);