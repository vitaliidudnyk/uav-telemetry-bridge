#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef void (*LoggerReader_OnBytesFn)(const uint8_t* data, size_t len, void* user_ctx);

typedef struct LoggerReaderConfig
{
  int uart_num;          // 1 or 2 (Serial1 / Serial2)
  int rx_pin;            // ESP32 RX pin connected to STM32 TX
  uint32_t baud;         // e.g. 115200
  size_t max_chunk;      // max bytes per callback, e.g. 256
} LoggerReaderConfig;

typedef struct LoggerReaderCallbacks
{
  LoggerReader_OnBytesFn on_bytes;
  void* user_ctx;
} LoggerReaderCallbacks;

void LoggerReader_Init(const LoggerReaderConfig* cfg, const LoggerReaderCallbacks* cbs);
void LoggerReader_Tick(uint32_t now_ms);
bool LoggerReader_IsReady();
