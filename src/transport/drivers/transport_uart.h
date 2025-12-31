#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct UartTransportConfig
{
  // Step later: which UART, pins, baud, etc.
  int placeholder;
} UartTransportConfig;

void UartTransport_Init(const UartTransportConfig* cfg);
void UartTransport_Tick(uint32_t now_ms);

bool UartTransport_IsReady();
bool UartTransport_Send(const uint8_t* data, size_t len);
