#pragma once

#include <stdbool.h>

void LoggerWriter_Init();
void LoggerWriter_SetEnabled(bool enabled);

void LoggerWriter_Info(const char* tag, const char* fmt, ...);
void LoggerWriter_Warn(const char* tag, const char* fmt, ...);
void LoggerWriter_Error(const char* tag, const char* fmt, ...);
