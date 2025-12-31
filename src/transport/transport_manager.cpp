#include "transport_manager.h"

#include "logging/logger_writer.h"
#include "transport/drivers/transport_wifi.h"
#include "transport/drivers/transport_ble.h"
#include "transport/drivers/transport_uart.h"

enum class ManagerState
{
  TRY_WIFI = 0,
  ACTIVE_WIFI,
  TRY_BLE,
  ACTIVE_BLE,
  TRY_UART,
  ACTIVE_UART
};

static TransportManagerConfig g_cfg = {};
static ManagerState g_state = ManagerState::TRY_WIFI;
static uint32_t g_state_start_ms = 0;

static void LogState(const char* msg)
{
  if (!g_cfg.enable_debug_logs)
  {
    return;
  }
  LoggerWriter_Info("TM", "%s", msg);
}

static void SetState(ManagerState next, uint32_t now_ms)
{
  g_state = next;
  g_state_start_ms = now_ms;

  switch (g_state)
  {
    case ManagerState::TRY_WIFI:    LogState("State=TRY_WIFI"); break;
    case ManagerState::ACTIVE_WIFI: LogState("State=ACTIVE_WIFI"); break;
    case ManagerState::TRY_BLE:     LogState("State=TRY_BLE"); break;
    case ManagerState::ACTIVE_BLE:  LogState("State=ACTIVE_BLE"); break;
    case ManagerState::TRY_UART:    LogState("State=TRY_UART"); break;
    case ManagerState::ACTIVE_UART: LogState("State=ACTIVE_UART"); break;
  }
}

static uint32_t ElapsedMs(uint32_t now_ms)
{
  return (uint32_t)(now_ms - g_state_start_ms);
}

void TransportManager_Init(const TransportManagerConfig* cfg)
{
  if (cfg != nullptr)
  {
    g_cfg = *cfg;
  }

  // Init drivers (configs are placeholders for now)
  WifiTransportConfig wcfg = {};
  BleTransportConfig  bcfg = {};
  UartTransportConfig ucfg = {};

  WifiTransport_Init(&wcfg);
  BleTransport_Init(&bcfg);
  UartTransport_Init(&ucfg);

  SetState(ManagerState::TRY_WIFI, 0);
}

void TransportManager_Tick(uint32_t now_ms)
{
  // Let drivers do periodic work (even if stubs now)
  WifiTransport_Tick(now_ms);
  BleTransport_Tick(now_ms);
  UartTransport_Tick(now_ms);

  switch (g_state)
  {
    case ManagerState::TRY_WIFI:
    {
      if (WifiTransport_IsReady())
      {
        SetState(ManagerState::ACTIVE_WIFI, now_ms);
      }
      else if (ElapsedMs(now_ms) >= g_cfg.wifi_try_ms)
      {
        SetState(ManagerState::TRY_BLE, now_ms);
      }
      break;
    }

    case ManagerState::ACTIVE_WIFI:
    {
      if (!WifiTransport_IsReady())
      {
        LogState("WiFi lost -> TRY_BLE");
        SetState(ManagerState::TRY_BLE, now_ms);
      }
      break;
    }

    case ManagerState::TRY_BLE:
    {
      if (BleTransport_IsReady())
      {
        SetState(ManagerState::ACTIVE_BLE, now_ms);
      }
      else if (ElapsedMs(now_ms) >= g_cfg.ble_try_ms)
      {
        if (g_cfg.allow_uart_transport)
        {
          SetState(ManagerState::TRY_UART, now_ms);
        }
        else
        {
          SetState(ManagerState::TRY_WIFI, now_ms);
        }
      }
      break;
    }

    case ManagerState::ACTIVE_BLE:
    {
      if (!BleTransport_IsReady())
      {
        LogState("BLE lost -> TRY_WIFI");
        SetState(ManagerState::TRY_WIFI, now_ms);
      }
      break;
    }

    case ManagerState::TRY_UART:
    {
      if (UartTransport_IsReady())
      {
        SetState(ManagerState::ACTIVE_UART, now_ms);
      }
      else
      {
        // No timer needed here for now: UART is last resort; loop back to WiFi to retry
        SetState(ManagerState::TRY_WIFI, now_ms);
      }
      break;
    }

    case ManagerState::ACTIVE_UART:
    {
      if (!UartTransport_IsReady())
      {
        LogState("UART lost -> TRY_WIFI");
        SetState(ManagerState::TRY_WIFI, now_ms);
      }
      break;
    }
  }
}

TransportKind TransportManager_GetActiveTransport()
{
  switch (g_state)
  {
    case ManagerState::ACTIVE_WIFI: return TRANSPORT_KIND_WIFI;
    case ManagerState::ACTIVE_BLE:  return TRANSPORT_KIND_BLE;
    case ManagerState::ACTIVE_UART: return TRANSPORT_KIND_UART;
    default:                        return TRANSPORT_KIND_NONE;
  }
}

bool TransportManager_SendBytes(const uint8_t* data, size_t len)
{
  if (data == nullptr || len == 0)
  {
    return false;
  }

  switch (g_state)
  {
    case ManagerState::ACTIVE_WIFI:
      return WifiTransport_Send(data, len);

    case ManagerState::ACTIVE_BLE:
      return BleTransport_Send(data, len);

    case ManagerState::ACTIVE_UART:
      return UartTransport_Send(data, len);

    default:
      return false;
  }
}
