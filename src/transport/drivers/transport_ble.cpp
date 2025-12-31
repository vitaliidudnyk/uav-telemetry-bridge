#include "transport_ble.h"

#include <Arduino.h>

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#include "logging/logger_writer.h"

// Nordic UART Service (NUS) UUIDs (commonly used, easy to debug)
static const char* BLE_DEVICE_NAME = "LogBridge";

static const char* NUS_SERVICE_UUID = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";
static const char* NUS_RX_UUID      = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"; // write from client -> device (optional)
static const char* NUS_TX_UUID      = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"; // notify from device -> client (future)

static BLEServer* g_server = nullptr;
static BLEService* g_service = nullptr;
static BLECharacteristic* g_tx_char = nullptr;
static BLECharacteristic* g_rx_char = nullptr;

static bool g_inited = false;
static volatile bool g_connected = false;

static void LogI(const char* fmt, ...)
{
  char buf[256];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  LoggerWriter_Info("BLE", "%s", buf);
}

static void LogW(const char* fmt, ...)
{
  char buf[256];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  LoggerWriter_Warn("BLE", "%s", buf);
}

class ServerCallbacks final : public BLEServerCallbacks
{
  void onConnect(BLEServer* pServer) override
  {
    (void)pServer;
    g_connected = true;
    LogI("Client connected");
  }

  void onDisconnect(BLEServer* pServer) override
  {
    (void)pServer;
    g_connected = false;
    LogW("Client disconnected, restarting advertising");
    BLEDevice::startAdvertising();
  }
};

void BleTransport_Init(const BleTransportConfig* cfg)
{
  (void)cfg;

  g_inited = false;
  g_connected = false;

  BLEDevice::init(BLE_DEVICE_NAME);

  g_server = BLEDevice::createServer();
  g_server->setCallbacks(new ServerCallbacks());

  g_service = g_server->createService(NUS_SERVICE_UUID);

  // TX characteristic (Notify) - will be used in Step C
  g_tx_char = g_service->createCharacteristic(
      NUS_TX_UUID,
      BLECharacteristic::PROPERTY_NOTIFY
  );
  g_tx_char->addDescriptor(new BLE2902());

  // RX characteristic (Write) - optional, but useful later if you want commands from Mac
  g_rx_char = g_service->createCharacteristic(
      NUS_RX_UUID,
      BLECharacteristic::PROPERTY_WRITE
  );

  g_service->start();

  BLEAdvertising* adv = BLEDevice::getAdvertising();
  adv->addServiceUUID(NUS_SERVICE_UUID);
  adv->setScanResponse(true);

  // Helps iOS/macOS in some cases; harmless on others
  adv->setMinPreferred(0x06);
  adv->setMinPreferred(0x12);

  BLEDevice::startAdvertising();

  g_inited = true;
  LogI("Init done. Advertising as '%s'", BLE_DEVICE_NAME);
}

void BleTransport_Tick(uint32_t now_ms)
{
  (void)now_ms;
  // No periodic work needed for Step B
}

bool BleTransport_IsReady()
{
  return g_inited && g_connected;
}

bool BleTransport_Send(const uint8_t* data, size_t len)
{
  if (!BleTransport_IsReady())
  {
    return false;
  }

  if (data == nullptr || len == 0)
  {
    return false;
  }

  if (g_tx_char == nullptr)
  {
    return false;
  }

  // BLE notifications have practical limits; keep chunks small.
  // We'll use a conservative cap here.
  const size_t max_notify = 180;
  size_t offset = 0;

  while (offset < len)
  {
    size_t chunk = len - offset;
    if (chunk > max_notify)
    {
      chunk = max_notify;
    }

    g_tx_char->setValue((uint8_t*)(data + offset), chunk);
    g_tx_char->notify();

    offset += chunk;

    // Small yield to avoid starving WiFi/RTOS tasks (even if WiFi is off)
    delay(1);
  }

  return true;
}

