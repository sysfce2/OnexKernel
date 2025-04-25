
#include <stdio.h>
#include <stdarg.h>

#include <onex-kernel/log.h>

bool log_to_serial=false;
bool log_to_gfx=false;

void log_init(properties* config) {
  log_to_serial = list_has_value(properties_get(config, "flags"), "log-to-serial");
  log_to_gfx    = list_has_value(properties_get(config, "flags"), "log-to-gfx");
}

bool log_loop()
{
  return false;
}

int16_t log_write_current_file_line(char* file, uint32_t line, const char* fmt, ...) {
  if(file) printf("%s:%d:", file, line);
  va_list args;
  va_start(args, fmt);
  int16_t r=vfprintf(stdout, fmt, args);
  va_end(args);
  return r;
}

void log_flush()
{
}
