#pragma once

#include <stdint.h>

void mr24hpc_uof_handle_frame(uint8_t ctrl, uint8_t cmd, const uint8_t *data, uint16_t len);