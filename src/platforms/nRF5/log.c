
#include <stdio.h>
#include <stdarg.h>

#include "app_error.h"
#include "nrf_log_ctrl.h"

#if defined(LOG_TO_SERIAL)
#include <onex-kernel/serial.h>
#include <onex-kernel/time.h>
#elif defined(LOG_TO_GFX)
#if defined(BOARD_PINETIME)
#include <onex-kernel/gfx.h>
#endif
#elif defined(LOG_TO_BLE)
#include <onex-kernel/blenus.h>
#endif
#include <onex-kernel/log.h>

void log_init()
{
  APP_ERROR_CHECK(NRF_LOG_INIT(NULL));
}

#if !defined(LOG_TO_SERIAL)
#define LOG_BUF_SIZE 1024
static char log_buf[LOG_BUF_SIZE];
#endif

int log_write(const char* fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  int r=0;
#if defined(LOG_TO_SERIAL)
//size_t n=snprintf((char*)log_buf, LOG_BUF_SIZE, "LOG: %s", fmt);
//if(n>=LOG_BUF_SIZE) n=LOG_BUF_SIZE-1;
  r=serial_vprintf(fmt, args);
  time_delay_ms(1);
#elif defined(LOG_TO_GFX)
#if defined(BOARD_PINETIME)
  vsnprintf((char*)log_buf, LOG_BUF_SIZE, fmt, args);
  char* nl=strchr(log_buf, '\n');
  char* s2;
  if(nl){
    *nl=0;
    s2=nl+1;
  }
  if(strlen(log_buf)>9) log_buf[9]=0;
  if(nl && strlen(s2)>9) s2[9]=0;
  gfx_push(10,150);
  gfx_text(log_buf);
  if(nl){
    gfx_push(10,190);
    gfx_text(s2);
    gfx_pop();
  }
  gfx_pop();
#endif
#elif defined(LOG_TO_BLE)
  vsnprintf((char*)log_buf, LOG_BUF_SIZE, fmt, args);
  //if(strlen(log_buf)>19){ log_buf[18]='\n'; log_buf[19]=0; }
  r=blenus_printf(log_buf);
#endif
  va_end(args);
  return r;
}


