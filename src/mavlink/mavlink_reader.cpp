#include "mavlink_reader.h"

static Stream* s_serial = nullptr;
static RingBuffer* s_fifo = nullptr;
static MavlinkStats* s_stats = nullptr;

// Step 1: raw bytes only (no parsing)
static constexpr size_t READ_CHUNK_MAX = 256;
static uint8_t s_read_buf[READ_CHUNK_MAX];

void MavlinkReader_Init(const MavlinkReaderConfig& cfg)
{
  s_serial = cfg.serial;
  s_fifo = cfg.fifo;
  s_stats = cfg.stats;
}

void MavlinkReader_Tick()
{
  if (s_serial == nullptr || s_fifo == nullptr) { return; }

  size_t budget = 1024; // bytes per tick max
  while (budget > 0)
  {
    int avail = s_serial->available();
    if (avail <= 0) { break; }

    size_t to_read = (size_t)avail;
    if (to_read > READ_CHUNK_MAX) { to_read = READ_CHUNK_MAX; }
    if (to_read > budget) { to_read = budget; }

    size_t got = s_serial->readBytes(s_read_buf, to_read);
    if (got == 0) { break; }

    size_t written = RingBuffer_Write(s_fifo, s_read_buf, got);
    if (s_stats) {
      s_stats->bytes_in += got;
      if (written < got) { s_stats->drops += (uint32_t)(got - written); }
    }

    budget -= got;
  }
}
