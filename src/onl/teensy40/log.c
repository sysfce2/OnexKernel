
#include <stdio.h>
#include <stdarg.h>

#if defined(LOG_TO_SERIAL)
#include <onex-kernel/serial.h>
#endif
#include <onex-kernel/time.h>
#include <onex-kernel/log.h>

void log_init()
{
}

bool log_loop()
{
  return false;
}

#if defined(LOG_TO_GFX)
#define LOG_BUF_SIZE 1024
static volatile char log_buffer[LOG_BUF_SIZE];
volatile char* event_log_buffer=0;
#endif

int log_write_current_file_line(char* file, uint32_t line, const char* fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  int r=0;
#if defined(LOG_TO_SERIAL)
//size_t n=snprintf((char*)log_buffer, LOG_BUF_SIZE, "LOG: %s", fmt);
//if(n>=LOG_BUF_SIZE) n=LOG_BUF_SIZE-1;
  r=serial_vprintf(fmt, args);
  time_delay_ms(1);
#elif defined(LOG_TO_GFX)
  uint16_t ln=file? snprintf((char*)log_buffer, LOG_BUF_SIZE, "%s:%ld:", file, line): 0;
  vsnprintf((char*)log_buffer+ln, LOG_BUF_SIZE-ln, fmt, args);
  event_log_buffer=log_buffer;
#endif
  va_end(args);
  return r;
}

void log_flush()
{
}


