#include "log_stream_flow.h"

#include <Arduino.h>

#include "common/ring_buffer.h"
#include "logging/logger_reader.h"
#include "logging/logger_writer.h"
#include "transport/transport_manager.h"

static constexpr size_t LOG_FIFO_CAPACITY = 16 * 1024;
static constexpr size_t FLUSH_CHUNK_MAX = 1024;
static constexpr uint32_t STATS_PERIOD_MS = 5000;

static RingBuffer g_fifo = {};
static uint8_t g_fifo_storage[LOG_FIFO_CAPACITY];

static uint32_t g_stats_last_ms = 0;
static uint32_t g_queued_total = 0;
static uint32_t g_dropped_total = 0;
static uint32_t g_dropped_last_total = 0;
static size_t g_fifo_max_used = 0;

static uint8_t g_flush_buf[FLUSH_CHUNK_MAX];

static void InitLoggerReaderConfig(LoggerReaderConfig* cfg)
{
  if (cfg == nullptr)
  {
    return;
  }

  cfg->uart_num = 2;
  cfg->rx_pin = 44;        // ESP32 RX pin connected to STM32 TX
  cfg->baud = 115200;      // Must match STM32 log UART baud
  cfg->max_chunk = 256;
}

static void InitTransportManagerConfig(TransportManagerConfig* cfg)
{
  if (cfg == nullptr)
  {
    return;
  }

  cfg->enable_debug_logs = true;     // Manager internal logs via LoggerWriter (if enabled)
  cfg->allow_uart_transport = true;  // Keep UART transport available if needed
  cfg->wifi_try_ms = 5000;
  cfg->ble_try_ms = 5000;
}

static void OnStm32LogBytes(const uint8_t* data, size_t len, void* user_ctx)
{
  (void)user_ctx;

  if (data == nullptr || len == 0)
  {
    return;
  }

  size_t written = RingBuffer_Write(&g_fifo, data, len);
  g_queued_total += (uint32_t)written;

  if (written < len)
  {
    g_dropped_total += (uint32_t)(len - written);
  }

  const size_t used = RingBuffer_Size(&g_fifo);
  if (used > g_fifo_max_used)
  {
    g_fifo_max_used = used;
  }
}

static void FlushFifoOnce()
{
  size_t to_send = RingBuffer_Peek(&g_fifo, g_flush_buf, sizeof(g_flush_buf));
  if (to_send == 0)
  {
    return;
  }

  bool ok = TransportManager_SendBytes(g_flush_buf, to_send);
  if (!ok)
  {
    return;
  }

  (void)RingBuffer_Skip(&g_fifo, to_send);
}

void LogStreamFlow_Init()
{
  (void)RingBuffer_Init(&g_fifo, g_fifo_storage, sizeof(g_fifo_storage));

  TransportManagerConfig tm_cfg = {};
  InitTransportManagerConfig(&tm_cfg);
  TransportManager_Init(&tm_cfg);

  LoggerReaderConfig lr_cfg = {};
  InitLoggerReaderConfig(&lr_cfg);

  LoggerReaderCallbacks lr_cbs = {};
  lr_cbs.on_bytes = &OnStm32LogBytes;
  lr_cbs.user_ctx = nullptr;

  LoggerReader_Init(&lr_cfg, &lr_cbs);

  g_stats_last_ms = (uint32_t)millis();

  LoggerWriter_Info("APP", "LogStreamFlow_Init done. fifo=%u", (unsigned)RingBuffer_Capacity(&g_fifo));
}

void LogStreamFlow_Tick(uint32_t now_ms)
{
  TransportManager_Tick(now_ms);
  LoggerReader_Tick(now_ms);

  // Drain a bit each tick without blocking too long
  for (int i = 0; i < 4; i++)
  {
    FlushFifoOnce();
    if (RingBuffer_Size(&g_fifo) == 0)
    {
      break;
    }
  }

  const uint32_t dropped_delta = g_dropped_total - g_dropped_last_total;

  const bool time_to_report = ((now_ms - g_stats_last_ms) >= STATS_PERIOD_MS);
  const bool has_new_drops = (dropped_delta > 0);

  if (time_to_report || has_new_drops)
  {
    g_stats_last_ms = now_ms;
    g_dropped_last_total = g_dropped_total;

    const size_t used = RingBuffer_Size(&g_fifo);
    const size_t free = RingBuffer_Free(&g_fifo);
    const TransportKind tk = TransportManager_GetActiveTransport();

    LoggerWriter_Info("APP", "transport=%d used=%u free=%u peak=%u queued=%lu dropped=%lu (+%lu)",
                      (int)tk,
                      (unsigned)used,
                      (unsigned)free,
                      (unsigned)g_fifo_max_used,
                      (unsigned long)g_queued_total,
                      (unsigned long)g_dropped_total,
                      (unsigned long)dropped_delta);
  }
}
