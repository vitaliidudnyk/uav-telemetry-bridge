#include "mavlink_relay_flow.h"

#include <Arduino.h>

#include "common/ring_buffer.h"
#include "logging/logger_writer.h"

#include "mavlink/mavlink_reader.h"
#include "mavlink/mavlink_relay.h"
#include "mavlink/mavlink_stats.h"

// Step 1: raw bytes relay (no MAVLink parsing)
static constexpr uint32_t STATS_PERIOD_MS = 5000;
static constexpr size_t MAV_FIFO_CAPACITY = 8 * 1024;

// UART plan:
// - RX from Mac via USB-UART: use Serial (UART0)
// - TX to STM32: use Serial1 with TX pin = GPIO43
static constexpr uint32_t MAV_BAUD = 115200;

// Serial (UART0) is the USB-UART in most Arduino-ESP32 setups.
static Stream* const MAV_RX_SERIAL = &Serial;

// Serial1 will be used only for TX to STM32 (GPIO43).
static constexpr int MAV_TX_UART_NUM = 1;
static constexpr int MAV_TX_PIN = 43;
static constexpr int MAV_TX_RX_PIN_UNUSED = -1;

static HardwareSerial g_mav_tx_serial(MAV_TX_UART_NUM);

static RingBuffer g_mav_fifo = {};
static uint8_t g_mav_fifo_storage[MAV_FIFO_CAPACITY];

static MavlinkStats g_mav_stats = {};
static uint32_t g_stats_last_ms = 0;

static void LogStats()
{
  const size_t used = RingBuffer_Size(&g_mav_fifo);
  const size_t free = RingBuffer_Free(&g_mav_fifo);

  LoggerWriter_Info("MAV", "in=%llu out=%llu drops=%lu fifo_used=%u fifo_free=%u",
                    (unsigned long long)g_mav_stats.bytes_in,
                    (unsigned long long)g_mav_stats.bytes_out,
                    (unsigned long)g_mav_stats.drops,
                    (unsigned)used,
                    (unsigned)free);
}

void MavlinkRelayFlow_Init()
{
  (void)RingBuffer_Init(&g_mav_fifo, g_mav_fifo_storage, sizeof(g_mav_fifo_storage));

  // TX (ESP32 -> STM32) via GPIO43.
  // We don't use RX on this UART in step 1, so set rxPin = -1.
  g_mav_tx_serial.begin(MAV_BAUD, SERIAL_8N1, MAV_TX_RX_PIN_UNUSED, MAV_TX_PIN);

  MavlinkReaderConfig r_cfg = {};
  r_cfg.serial = MAV_RX_SERIAL;
  r_cfg.fifo = &g_mav_fifo;
  r_cfg.stats = &g_mav_stats;
  MavlinkReader_Init(r_cfg);

  MavlinkRelayConfig t_cfg = {};
  t_cfg.fifo = &g_mav_fifo;
  t_cfg.out_serial = &g_mav_tx_serial;
  t_cfg.stats = &g_mav_stats;
  MavlinkRelay_Init(t_cfg);

  g_stats_last_ms = (uint32_t)millis();

  LoggerWriter_Info("MAV", "MavlinkRelayFlow_Init done. fifo=%u baud=%lu tx_uart=%d tx_pin=%d",
                    (unsigned)RingBuffer_Capacity(&g_mav_fifo),
                    (unsigned long)MAV_BAUD,
                    (int)MAV_TX_UART_NUM,
                    (int)MAV_TX_PIN);
}

void MavlinkRelayFlow_Tick(uint32_t now_ms)
{
  MavlinkReader_Tick();
  MavlinkRelay_Tick();

  if ((now_ms - g_stats_last_ms) >= STATS_PERIOD_MS)
  {
    g_stats_last_ms = now_ms;
    LogStats();
  }
}
