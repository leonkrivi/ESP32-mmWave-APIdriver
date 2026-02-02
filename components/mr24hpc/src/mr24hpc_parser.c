#include <stdint.h>
#include <string.h>

#include "esp_log.h"
#include "esp_timer.h"

#include "mr24hpc.h"
#include "mr24hpc_types.h"
#include "internal.h"

#define MR24_HEADER_1 0x53
#define MR24_HEADER_2 0x59

static float static_dist_to_m(uint8_t code);
static float motion_dist_to_m(uint8_t code);
static float motion_speed_to_m_s(uint8_t code);
static mr24hpc_direction_t speed_to_direction(uint8_t code);

static mr24hpc_parser_t parser;

void mr24hpc_parser_init(void) {
    parser = (mr24hpc_parser_t){0};
    parser.ps = WAIT_H1;
}

void mr24hpc_parser_feed(uint8_t byte) {
    static uint8_t checksum_buf[256];
    static size_t checksum_len = 0;

    switch (parser.ps) {

    case WAIT_H1:
        if (byte == MR24_HEADER_1) {
            checksum_len = 0;
            checksum_buf[checksum_len++] = byte;
            parser.ps = WAIT_H2;
        }
        break;

    case WAIT_H2:
        if (byte == MR24_HEADER_2) {
            checksum_buf[checksum_len++] = byte;
            parser.ps = WAIT_CTRL;
        } else {
            parser.ps = WAIT_H1;
        }
        break;

    case WAIT_CTRL:
        parser.ctrl = byte;
        checksum_buf[checksum_len++] = byte;
        parser.ps = WAIT_CMD;
        break;

    case WAIT_CMD:
        parser.cmd = byte;
        checksum_buf[checksum_len++] = byte;
        parser.ps = WAIT_LEN1;
        break;

    case WAIT_LEN1:
        // MSB first (big-endian) — start fresh to avoid leftover bits
        parser.len = ((uint16_t)byte << 8) & 0xFF00;
        checksum_buf[checksum_len++] = byte;
        parser.ps = WAIT_LEN2;
        break;

    case WAIT_LEN2:
        // LSB — explicitly overwrite low byte
        parser.len = (parser.len & 0xFF00) | (uint16_t)byte;
        checksum_buf[checksum_len++] = byte;
        parser.data_idx = 0;
        if (parser.len == 0)
            parser.ps = WAIT_CHECKSUM;
        else if (parser.len <= (uint16_t)sizeof(parser.data))
            parser.ps = WAIT_DATA;
        else
            parser.ps = WAIT_H1; // invalid/overflow
        break;

    case WAIT_DATA:
        if (parser.data_idx < (uint16_t)sizeof(parser.data)) {
            parser.data[parser.data_idx++] = byte;
            checksum_buf[checksum_len++] = byte;
        } else {
            parser.ps = WAIT_H1;
            break;
        }

        if (parser.data_idx >= parser.len)
            parser.ps = WAIT_CHECKSUM;
        break;

    case WAIT_CHECKSUM:
        uint8_t chksum = calculate_checksum(checksum_buf, checksum_len);
        if (chksum == byte) {
            handle_frame(parser.ctrl, parser.cmd, parser.data, parser.len);
        }
        parser.ps = WAIT_H1;
        break;

    }
}

void handle_frame(uint8_t ctrl, uint8_t cmd, const uint8_t *data, uint16_t len)
{
    // Underlying Open Function uses CTRL = 0x08
    if (ctrl != 0x08) {
        return;
    }

    mr24hpc_state_t update = {0};

    switch (cmd) {

    /* ============ Full sensor report (proactive) ============
     * CMD = 0x01, LEN = 5
     * byte0: existence energy (0-250)
     * byte1: static distance (0x01-0x06)
     * byte2: motion energy (0-250)
     * byte3: motion distance (0x01-0x08)
     * byte4: motion speed (0x01-0x14)
     */
    case 0x01:
        if (len < 5) return;
        update.existence_energy = data[0];
        update.static_distance_m = static_dist_to_m(data[1]);
        update.motion_energy = data[2];
        update.motion_distance_m = motion_dist_to_m(data[3]);
        update.motion_speed_m_s = motion_speed_to_m_s(data[4]);
        update.direction = speed_to_direction(data[4]);
        update.valid_mask = MR24HPC_VALID_EXISTENCE_ENERGY
                          | MR24HPC_VALID_STATIC_DISTANCE
                          | MR24HPC_VALID_MOTION_ENERGY
                          | MR24HPC_VALID_MOTION_DISTANCE
                          | MR24HPC_VALID_MOTION_SPEED
                          | MR24HPC_VALID_DIRECTION;
        break;

    /* ============ Inquiry responses (CMD 0x81-0x87) ============ */

    /* CMD 0x81: Existence energy response (1 byte, 0-250) */
    case 0x81:
        if (len < 1) return;
        update.existence_energy = data[0];
        update.valid_mask = MR24HPC_VALID_EXISTENCE_ENERGY;
        break;

    /* CMD 0x82: Motion energy response (1 byte, 0-250) */
    case 0x82:
        if (len < 1) return;
        update.motion_energy = data[0];
        update.valid_mask = MR24HPC_VALID_MOTION_ENERGY;
        break;

    /* CMD 0x83: Static distance response (1 byte) */
    case 0x83:
        if (len < 1) return;
        update.static_distance_m = static_dist_to_m(data[0]);
        update.valid_mask = MR24HPC_VALID_STATIC_DISTANCE;
        break;

    /* CMD 0x84: Motion distance response (1 byte) */
    case 0x84:
        if (len < 1) return;
        update.motion_distance_m = motion_dist_to_m(data[0]);
        update.valid_mask = MR24HPC_VALID_MOTION_DISTANCE;
        break;

    /* CMD 0x85: Motion speed response (1 byte) */
    case 0x85:
        if (len < 1) return;
        update.motion_speed_m_s = motion_speed_to_m_s(data[0]);
        update.direction = speed_to_direction(data[0]);
        update.valid_mask = MR24HPC_VALID_MOTION_SPEED | MR24HPC_VALID_DIRECTION;
        break;

    /* CMD 0x86: Approaching/moving away response (1 byte)
     * 0x00 = none/stationary, 0x01 = approaching, 0x02 = moving away */
    case 0x86:
        if (len < 1) return;
        update.direction = (mr24hpc_direction_t)data[0];
        update.valid_mask = MR24HPC_VALID_DIRECTION;
        break;

    /* CMD 0x87: Moving parameters response (1 byte, 0-100) */
    case 0x87:
        if (len < 1) return;
        update.moving_params = data[0];
        update.valid_mask = MR24HPC_VALID_MOVING_PARAMS;
        break;

    default:
        return;
    }

    update.last_update_ms = mr24hpc_ms_since_last_update();
    mr24hpc_update_state(&update);
}


// ================ helper functions ================

// Helper: convert static distance code to meters
// 0x00 = no one, 0x01 = 0.5m, 0x02 = 1.0m, ... 0x06 = 3.0m
static float static_dist_to_m(uint8_t code) {
    if (code == 0x00) return 0.0f;
    return code * 0.5f;
}

// Helper: convert motion distance code to meters
// 0x00 = no one moving, 0x01 = 0.5m, ... 0x08 = 4.0m
static float motion_dist_to_m(uint8_t code) {
    if (code == 0x00) return 0.0f;
    return code * 0.5f;
}

// Helper: convert motion speed code to m/s
// 0x0a = 0 m/s (stationary)
// 0x01-0x09: approaching, speed = (0x0a - code) * 0.5 m/s
// 0x0b-0x14: moving away, speed = -(code - 0x0a) * 0.5 m/s
// Returns signed speed (positive = approaching, negative = away)
static float motion_speed_to_m_s(uint8_t code) {
    if (code == 0x00 || code == 0x0a) return 0.0f;
    if (code >= 0x01 && code <= 0x09) {
        return (0x0a - code) * 0.5f;  // approaching: positive
    }
    if (code >= 0x0b && code <= 0x14) {
        return -((int)(code - 0x0a)) * 0.5f;  // moving away: negative
    }
    return 0.0f;
}

// Helper: get direction from speed code
static mr24hpc_direction_t speed_to_direction(uint8_t code) {
    if (code >= 0x01 && code <= 0x09) return MR24HPC_DIR_APPROACHING;
    if (code >= 0x0b && code <= 0x14) return MR24HPC_DIR_MOVING_AWAY;
    return MR24HPC_DIR_NONE;
}


uint8_t calculate_checksum(const uint8_t *data, size_t len) {
    uint32_t sum = 0;
    for (int i = 0; i < len; ++i) sum += data[i];
    return (uint8_t)(sum & 0xFF);
}

uint32_t mr24hpc_ms_since_last_update(void) {
    uint32_t now = (uint32_t)(esp_timer_get_time() / 1000ULL);
    return now - global_sensor_state.last_update_ms;
}