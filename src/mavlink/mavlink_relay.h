#pragma once
#include <Arduino.h>
#include "common/ring_buffer.h"
#include "mavlink_stats.h"

// Drains FIFO and writes raw bytes to STM32 UART
struct MavlinkRelayConfig
{
    RingBuffer* fifo = nullptr;
    HardwareSerial* out_serial = nullptr;
    MavlinkStats* stats = nullptr;
};

void MavlinkRelay_Init(const MavlinkRelayConfig& cfg);
void MavlinkRelay_Tick();
