#include <Arduino.h>
#include "app/app.h"
#include "logging/logger_writer.h"

static constexpr uint32_t USB_SERIAL_BAUD = 115200;

void setup()
{
  Serial.begin(USB_SERIAL_BAUD);

  LoggerWriter_Init();

  // For test sessions set to true (when MAVLink is off).
  // For normal operation set to false (when MAVLink occupies USB serial).
  LoggerWriter_SetEnabled(true);

  App_Init();
}

void loop()
{
  App_Tick();
  delay(1);
}
