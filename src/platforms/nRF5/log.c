
#include <stdio.h>
#include <stdarg.h>

#include "app_error.h"
#include "nrf_log_ctrl.h"

#if defined(LOG_TO_SERIAL)
#include <onex-kernel/serial.h>
#elif defined(LOG_TO_BLE)
#include <onex-kernel/blenus.h>
#endif
#include <onex-kernel/time.h>
#include <onex-kernel/log.h>

void log_init()
{
  APP_ERROR_CHECK(NRF_LOG_INIT(NULL));
}

#if defined(LOG_TO_BLE) || defined(LOG_TO_GFX)
#define LOG_BUF_SIZE 1024
static char log_buffer[LOG_BUF_SIZE];
#if defined(LOG_TO_GFX)
volatile char* event_log_buffer=0;
#endif
#endif

int log_write(const char* fmt, ...)
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
  vsnprintf(log_buffer, LOG_BUF_SIZE, fmt, args);
  event_log_buffer=log_buffer;
#elif defined(LOG_TO_BLE)
  vsnprintf((char*)log_buffer, LOG_BUF_SIZE, fmt, args);
  //if(strlen(log_buffer)>19){ log_buffer[18]='\n'; log_buffer[19]=0; }
  r=blenus_printf(log_buffer);
  time_delay_ms(5);
#endif
  va_end(args);
  return r;
}


