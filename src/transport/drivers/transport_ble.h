#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct BleTransportConfig
{
  // Step later: device name, service UUID, characteristic UUID, etc.
  int placeholder;
} BleTransportConfig;

void BleTransport_Init(const BleTransportConfig* cfg);
void BleTransport_Tick(uint32_t now_ms);

bool BleTransport_IsReady();
bool BleTransport_Send(const uint8_t* data, size_t len);
