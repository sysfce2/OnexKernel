
#include <stdio.h>
#include <stdarg.h>

#include <onex-kernel/serial.h>
#include <onex-kernel/log.h>

void log_init()
{
}

int log_write(const char* fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  int r=serial_vprintf(fmt, args);
  va_end(args);
  return r;
}


