
#include <stdio.h>
#include <stdarg.h>

#include <onex-kernel/log.h>

void log_init(uint32_t baudrate)
{
}

int log_write(const char* fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  int r=vfprintf(stdout, fmt, args);
  va_end(args);
  return r;
}

