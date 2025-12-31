#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "common/types.h"

typedef struct TransportManagerConfig
{
  bool enable_debug_logs;
  bool allow_uart_transport;
  uint32_t wifi_try_ms;
  uint32_t ble_try_ms;
} TransportManagerConfig;

void TransportManager_Init(const TransportManagerConfig* cfg);
void TransportManager_Tick(uint32_t now_ms);

TransportKind TransportManager_GetActiveTransport();
bool TransportManager_SendBytes(const uint8_t* data, size_t len);
