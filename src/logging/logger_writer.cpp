#include "logger_writer.h"

#include <Arduino.h>
#include <stdarg.h>
#include <stdio.h>

static bool g_enabled = false;

void LoggerWriter_Init()
{
  // No hardware init here by design.
}

void LoggerWriter_SetEnabled(bool enabled)
{
  g_enabled = enabled;
}

static void log_internal(const char* level, const char* tag, const char* fmt, va_list args)
{
  if (!g_enabled)
  {
    return;
  }

  char msg[256];
  vsnprintf(msg, sizeof(msg), fmt, args);

  Serial.printf("[%s][%s] %s\r\n", level, tag, msg);
}

void LoggerWriter_Info(const char* tag, const char* fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  log_internal("I", tag, fmt, args);
  va_end(args);
}

void LoggerWriter_Warn(const char* tag, const char* fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  log_internal("W", tag, fmt, args);
  va_end(args);
}

void LoggerWriter_Error(const char* tag, const char* fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  log_internal("E", tag, fmt, args);
  va_end(args);
}
