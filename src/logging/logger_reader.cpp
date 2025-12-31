#include "logger_reader.h"

#include <Arduino.h>

#include "logging/logger_writer.h"

static bool g_inited = false;
static LoggerReaderCallbacks g_cbs = {};

static HardwareSerial* g_uart = nullptr;
static HardwareSerial g_uart1(1);
static HardwareSerial g_uart2(2);

static LoggerReaderConfig g_cfg = {};

// One static buffer for chunk reads
static uint8_t g_buf[512];

void LoggerReader_Init(const LoggerReaderConfig* cfg, const LoggerReaderCallbacks* cbs)
{
  g_inited = false;
  g_uart = nullptr;
  g_cbs = {};

  if (cfg == nullptr || cbs == nullptr || cbs->on_bytes == nullptr)
  {
    LoggerWriter_Error("LR", "Init failed: invalid cfg/callbacks");
    return;
  }

  g_cfg = *cfg;
  g_cbs = *cbs;

  // Select UART
  if (g_cfg.uart_num == 1)
  {
    g_uart = &g_uart1;
  }
  else if (g_cfg.uart_num == 2)
  {
    g_uart = &g_uart2;
  }
  else
  {
    LoggerWriter_Error("LR", "Init failed: uart_num must be 1 or 2");
    return;
  }

  if (g_cfg.baud == 0)
  {
    LoggerWriter_Error("LR", "Init failed: baud must be > 0");
    return;
  }

  if (g_cfg.rx_pin < 0)
  {
    LoggerWriter_Error("LR", "Init failed: rx_pin must be set");
    return;
  }

  if (g_cfg.max_chunk == 0)
  {
    g_cfg.max_chunk = 256;
  }
  if (g_cfg.max_chunk > sizeof(g_buf))
  {
    g_cfg.max_chunk = sizeof(g_buf);
  }

  // Start UART (RX only is fine; tx_pin may be -1)
  g_uart->begin(g_cfg.baud, SERIAL_8N1, g_cfg.rx_pin);

  g_inited = true;
  LoggerWriter_Info("LR", "UART started: uart=%d baud=%lu rx=%d tx=%d chunk=%u",
                    g_cfg.uart_num,
                    (unsigned long)g_cfg.baud,
                    g_cfg.rx_pin,
                    (unsigned)g_cfg.max_chunk);
}

void LoggerReader_Tick(uint32_t now_ms)
{
  (void)now_ms;

  if (!g_inited || g_uart == nullptr)
  {
    return;
  }

  int avail = g_uart->available();
  if (avail <= 0)
  {
    return;
  }

  size_t to_read = (size_t)avail;
  if (to_read > g_cfg.max_chunk)
  {
    to_read = g_cfg.max_chunk;
  }

  // Non-blocking read: readBytes() will return quickly if data is available
  size_t n = g_uart->readBytes(g_buf, to_read);
  if (n > 0)
  {
    g_cbs.on_bytes(g_buf, n, g_cbs.user_ctx);
  }
}

bool LoggerReader_IsReady()
{
  return g_inited;
}
