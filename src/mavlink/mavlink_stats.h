#pragma once
#include <stdint.h>

// Simple statistics for MAVLink relay (step 1: no parsing)
struct MavlinkStats
{
    uint64_t bytes_in = 0;
    uint64_t bytes_out = 0;
    uint32_t drops = 0;

    uint32_t last_log_ms = 0;
};
