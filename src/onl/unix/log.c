
#include <stdio.h>
#include <stdarg.h>

#include <onex-kernel/log.h>

// not on Unix
bool log_to_gfx=false;
bool log_to_rtt=false;
bool log_to_led=false;
bool debug_on_serial=false;
bool log_onp=false;

void log_init(properties* config) {
  log_onp = list_vals_has(properties_get(config, "flags"), "log-onp");
}

bool log_loop() {
  return false;
}

int16_t log_write_mode(uint8_t mode, char* file, uint32_t line, const char* fmt, ...){

  va_list args;
  va_start(args, fmt);

  bool fl=(mode==1 || mode==3);
  bool nw=(mode==2 || mode==3);

  // if(!nw) return 0; // narrow down logging to only modes 2/3

  int16_t r=0;

  r+=fl? printf("[%d](%s:%d) ", (uint32_t)time_ms(), file, line): 0;
  r+=  vfprintf(stdout, fmt, args);

  va_end(args);
  return r;
}

bool log_debug_read(char* buf, uint16_t size){ return false; }
void log_flash_current_file_line(char* file, uint32_t line, uint8_t r, uint8_t g, uint8_t b){ }
void log_flush() { }


