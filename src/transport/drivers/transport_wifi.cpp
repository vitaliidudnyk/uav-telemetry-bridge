#include "transport_wifi.h"

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>

#include "logging/logger_writer.h"
#include "secrets.h"

static WiFiUDP g_udp;
static IPAddress g_target_ip;
static bool g_inited = false;

static uint32_t g_last_connect_attempt_ms = 0;
static bool g_was_connected = false;
static uint32_t g_send_error_streak = 0;
static uint32_t g_send_cooldown_until_ms = 0;
static uint32_t g_now_ms = 0;

static void LogI(const char* fmt, ...)
{
  char buf[256];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  LoggerWriter_Info("WIFI", "%s", buf);
}

static void LogW(const char* fmt, ...)
{
  char buf[256];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  LoggerWriter_Warn("WIFI", "%s", buf);
}

static void LogE(const char* fmt, ...)
{
  char buf[256];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  LoggerWriter_Error("WIFI", "%s", buf);
}

void WifiTransport_Init(const WifiTransportConfig* cfg)
{
  (void)cfg;

  if (!g_target_ip.fromString(UDP_TARGET_IP))
  {
    LogE("Invalid UDP_TARGET_IP: %s", UDP_TARGET_IP);
  }

  WiFi.mode(WIFI_STA);

  if (!WIFI_ENABLED)
  {
    WiFi.mode(WIFI_OFF);
    LogI("WiFi disabled by config");
    g_inited = true;
    return;
  }

  g_inited = true;
  g_last_connect_attempt_ms = 0;
  g_was_connected = false;

  LogI("Init: STA, target=%s:%u", UDP_TARGET_IP, (unsigned)UDP_TARGET_PORT);
}

void WifiTransport_Tick(uint32_t now_ms)
{
  if (!WIFI_ENABLED)
  {
    return;
  }

  if (!g_inited)
  {
    return;
  }

  g_now_ms = now_ms;

  const wl_status_t st = WiFi.status();
  const bool connected = (st == WL_CONNECTED);

  // Log connection edge
  if (connected && !g_was_connected)
  {
    g_was_connected = true;

    IPAddress ip = WiFi.localIP();
    int rssi = WiFi.RSSI();

    g_send_error_streak = 0;
    g_send_cooldown_until_ms = 0;

    LogI("Connected: ssid='%s' ip=%u.%u.%u.%u rssi=%d",
         WIFI_SSID, ip[0], ip[1], ip[2], ip[3], rssi);
    return;
  }
  if (!connected && g_was_connected)
  {
    g_was_connected = false;
    LogW("Disconnected");
  }

  if (connected)
  {
    return;
  }

  // Throttle reconnect attempts
  const uint32_t reconnect_period_ms = 3000;
  if ((now_ms - g_last_connect_attempt_ms) < reconnect_period_ms)
  {
    return;
  }
  g_last_connect_attempt_ms = now_ms;

  LogW("Connecting to SSID='%s'...", WIFI_SSID);

  // Best-effort restart WiFi connection
  WiFi.disconnect(true /* wifioff */);
  delay(10);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
}

bool WifiTransport_IsReady()
{
  if (!WIFI_ENABLED)
  {
    return false;
  }

  if (!g_inited)
  {
    return false;
  }

  if (WiFi.status() != WL_CONNECTED)
  {
    return false;
  }

  // If we recently had repeated UDP send failures, consider WiFi transport not ready
  if (g_now_ms < g_send_cooldown_until_ms)
  {
    return false;
  }

  return true;
}

bool WifiTransport_Send(const uint8_t* data, size_t len)
{
  if (!WifiTransport_IsReady())
  {
    return false;
  }

  if (data == nullptr || len == 0)
  {
    return false;
  }

  // Keep UDP payload modest. We'll improve later with buffering/segmentation if needed.
  int ok_begin = g_udp.beginPacket(g_target_ip, UDP_TARGET_PORT);
  if (ok_begin != 1)
  {
    return false;
  }

  size_t written = g_udp.write(data, len);
  int ok_end = g_udp.endPacket();
  const bool ok = (written == len) && (ok_end == 1);

  if (!ok)
  {
    g_send_error_streak++;

    // After a few consecutive failures, enter cooldown so manager can fallback to BLE.
    if (g_send_error_streak >= 5)
    {
      // 10 seconds cooldown
      g_send_cooldown_until_ms = g_now_ms + 10000;
      g_send_error_streak = 0;

      LoggerWriter_Warn("WIFI", "UDP send failing -> cooldown 10s (fallback to BLE expected)");
    }

    return false;
  }

  // Success resets streak
  g_send_error_streak = 0;
  return true;

}
