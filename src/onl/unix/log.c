
#include <stdio.h>
#include <stdarg.h>

#include <onex-kernel/log.h>

// not on Unix
bool log_to_gfx=false;
bool log_to_rtt=false;
bool log_to_led=false;
bool debug_on_serial=false;

// REVISIT: initialised?
void log_init(properties* config) {
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

bool log_debug_read(char* buf, uint16_t size){}
void log_flash(uint8_t r, uint8_t g, uint8_t b) { }
void log_flush() { }


