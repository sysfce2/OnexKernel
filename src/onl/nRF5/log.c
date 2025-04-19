
#include <stdio.h>
#include <stdarg.h>

#include "app_error.h"
#include "nrf_log_default_backends.h"

#include <onex-kernel/serial.h>
#include <onex-kernel/time.h>
#include <onex-kernel/log.h>

bool log_to_serial=false;
bool log_to_gfx=false;
bool log_to_rtt=false;

void log_init(properties* config) {

  log_to_serial = list_has_value(properties_get(config, "flags"), "log-to-serial");
  log_to_gfx    = list_has_value(properties_get(config, "flags"), "log-to-gfx");
  log_to_rtt    = list_has_value(properties_get(config, "flags"), "log-to-rtt");

  if(log_to_serial) serial_init(0,0);

#if defined(NRF_LOG_ENABLED)
  NRF_LOG_INIT(NULL);
  NRF_LOG_DEFAULT_BACKENDS_INIT();
#endif
}

bool log_loop()
{
#if defined(NRF_LOG_ENABLED)
  return NRF_LOG_PROCESS();
#else
  return false;
#endif
}

#define LOG_BUF_SIZE 1024
static volatile char log_buffer[LOG_BUF_SIZE];
volatile char* event_log_buffer=0;

int log_write_current_file_line(char* file, uint32_t line, const char* fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  int r=0;
  if(log_to_serial){
    //size_t n=snprintf((char*)log_buffer, LOG_BUF_SIZE, "LOG: %s", fmt);
    //if(n>=LOG_BUF_SIZE) n=LOG_BUF_SIZE-1;
    r=serial_vprintf(fmt, args);
    time_delay_ms(1);
  }
  if(log_to_gfx){
    uint16_t ln=file? snprintf((char*)log_buffer, LOG_BUF_SIZE, "%s:%ld:", file, line): 0;
    vsnprintf((char*)log_buffer+ln, LOG_BUF_SIZE-ln, fmt, args);
    event_log_buffer=log_buffer;
  }
  if(log_to_rtt){
    vsnprintf((char*)log_buffer, LOG_BUF_SIZE, fmt, args);
    NRF_LOG_DEBUG("%s", (char*)log_buffer);
    time_delay_ms(2);
    NRF_LOG_FLUSH();
    time_delay_ms(2);
  }
  va_end(args);
  return r;
}

void log_flush()
{
#if defined(NRF_LOG_ENABLED)
  NRF_LOG_FLUSH();
  time_delay_ms(5);
#endif
}


