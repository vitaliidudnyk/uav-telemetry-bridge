#include "mavlink_relay.h"

static RingBuffer* s_fifo = nullptr;
static HardwareSerial* s_out = nullptr;
static MavlinkStats* s_stats = nullptr;

// Step 1: raw bytes only (no parsing)
static constexpr size_t RELAY_CHUNK_MAX = 256;
static uint8_t s_relay_buf[RELAY_CHUNK_MAX];

void MavlinkRelay_Init(const MavlinkRelayConfig& cfg)
{
  s_fifo = cfg.fifo;
  s_out = cfg.out_serial;
  s_stats = cfg.stats;
}

void MavlinkRelay_Tick()
{
  if (s_fifo == nullptr || s_out == nullptr)
  {
    return;
  }

  // Drain FIFO in small chunks to avoid long blocking
  for (int i = 0; i < 4; i++)
  {
    size_t to_send = RingBuffer_Peek(s_fifo, s_relay_buf, sizeof(s_relay_buf));
    if (to_send == 0)
    {
      break;
    }

    // Serial.write can be partial; handle that safely
    size_t written = s_out->write(s_relay_buf, to_send);
    if (written == 0)
    {
      // TX not ready / blocked; try later
      break;
    }

    (void)RingBuffer_Skip(s_fifo, written);

    if (s_stats != nullptr)
    {
      s_stats->bytes_out += (uint64_t)written;
    }

    if (written < to_send)
    {
      // Partial send happened; stop this tick to avoid busy loop
      break;
    }
  }
}
