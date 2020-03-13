
#include <stdio.h>
#include <stdarg.h>

#include "app_error.h"
#include "nrf_log_ctrl.h"

#if defined(HAS_SERIAL)
#include <onex-kernel/serial.h>
#elif defined(LOG_TO_GFX)
#if defined(BOARD_PINETIME)
#include <onex-kernel/gfx.h>
#endif
#else
#include <onex-kernel/blenus.h>
#endif
#include <onex-kernel/log.h>

void log_init()
{
  APP_ERROR_CHECK(NRF_LOG_INIT(NULL));
}

#if !defined(HAS_SERIAL)
#define LOG_BUF_SIZE 1024
static char log_buf[LOG_BUF_SIZE];
#endif

int log_write(const char* fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  int r=0;
#if defined(HAS_SERIAL)
//size_t n=snprintf((char*)log_buf, LOG_BUF_SIZE, "LOG: %s", fmt);
//if(n>=LOG_BUF_SIZE) n=LOG_BUF_SIZE-1;
  r=serial_vprintf(fmt, args);
#elif defined(LOG_TO_GFX)
#if defined(BOARD_PINETIME)
  vsnprintf((char*)log_buf, LOG_BUF_SIZE, fmt, args);
  if(strlen(log_buf)>9) log_buf[9]=0;
  gfx_push(200,10);
  gfx_text(log_buf);
  gfx_pop();
#endif
#else
  vsnprintf((char*)log_buf, LOG_BUF_SIZE, fmt, args);
  if(strlen(log_buf)>19){ log_buf[18]='\n'; log_buf[19]=0; }
  r=blenus_printf(log_buf);
#endif
  va_end(args);
  return r;
}


