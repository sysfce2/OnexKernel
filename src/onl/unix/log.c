
#include <stdio.h>
#include <stdarg.h>

#include <onex-kernel/log.h>

void log_init()
{
}

bool log_loop()
{
  return false;
}

int log_write_current_file_line(char* file, uint32_t line, const char* fmt, ...) {
  if(file) printf("%s:%d:", file, line);
  va_list args;
  va_start(args, fmt);
  int r=vfprintf(stdout, fmt, args);
  va_end(args);
  return r;
}

void log_flush()
{
}
