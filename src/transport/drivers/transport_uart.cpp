#include "transport_uart.h"

void UartTransport_Init(const UartTransportConfig* cfg)
{
  (void)cfg;
}

void UartTransport_Tick(uint32_t now_ms)
{
  (void)now_ms;
}

bool UartTransport_IsReady()
{
  return false;
}

bool UartTransport_Send(const uint8_t* data, size_t len)
{
  (void)data;
  (void)len;
  return false;
}
