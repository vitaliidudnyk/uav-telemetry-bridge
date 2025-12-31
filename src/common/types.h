#pragma once

#include <stdint.h>

typedef enum TransportKind
{
  TRANSPORT_KIND_NONE = 0,
  TRANSPORT_KIND_WIFI,
  TRANSPORT_KIND_BLE,
  TRANSPORT_KIND_UART
} TransportKind;

typedef struct TimeMs
{
  uint32_t value;
} TimeMs;
