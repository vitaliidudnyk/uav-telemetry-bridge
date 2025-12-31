#pragma once
#include <Arduino.h>
#include "common/ring_buffer.h"
#include "mavlink_stats.h"

// Reads raw bytes from UART and pushes them into FIFO
struct MavlinkReaderConfig
{
    Stream* serial = nullptr;
    RingBuffer* fifo = nullptr;
    MavlinkStats* stats = nullptr;
};

void MavlinkReader_Init(const MavlinkReaderConfig& cfg);
void MavlinkReader_Tick();
