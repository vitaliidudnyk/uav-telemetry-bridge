#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct WifiTransportConfig
{
  // Step later: SSID/pass, UDP target, port, etc.
  int placeholder;
} WifiTransportConfig;

void WifiTransport_Init(const WifiTransportConfig* cfg);
void WifiTransport_Tick(uint32_t now_ms);

bool WifiTransport_IsReady();
bool WifiTransport_Send(const uint8_t* data, size_t len);
