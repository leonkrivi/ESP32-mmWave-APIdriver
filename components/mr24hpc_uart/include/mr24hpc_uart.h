#pragma once

#include <stddef.h>
#include <stdint.h>

void uart_init(void);
int uart_read(uint8_t *buf, size_t len, uint32_t timeout_ms);
int uart_write(const uint8_t *buf, size_t len);

// void uart_lock(void);
// void uart_unlock(void);