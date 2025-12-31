#include "app.h"

#include <Arduino.h>

#include "app/flows/log_stream_flow.h"
#include "app/flows/mavlink_relay_flow.h"
#include "logging/logger_writer.h"

void App_Init()
{
  LogStreamFlow_Init();
  MavlinkRelayFlow_Init();

  LoggerWriter_Info("APP", "App_Init done.");
}

void App_Tick()
{
  const uint32_t now_ms = (uint32_t)millis();

  LogStreamFlow_Tick(now_ms);
  MavlinkRelayFlow_Tick(now_ms);
}
