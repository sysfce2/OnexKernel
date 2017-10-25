
#include <stdio.h>
#include <stdarg.h>

#include <onex-kernel/serial.h>
#include <onex-kernel/log.h>

static bool initialised=false;

void log_init(uint32_t baudrate)
{
#if !defined(ONP_CHANNEL_SERIAL)
  if(initialised) return;
  serial_init(0, baudrate);
  initialised=true;
#endif
}

int log_write(const char* fmt, ...)
{
#if !defined(ONP_CHANNEL_SERIAL)
  if(!initialised) log_init(0);
  va_list args;
  va_start(args, fmt);
  int r=vfprintf(stdout, fmt, args);
  va_end(args);
  return r;
#else
  return 0;
#endif
}


