
#include <stdio.h>
#include <stdarg.h>

#include "app_error.h"
#include "nrf_log_ctrl.h"

#ifdef HAS_SERIAL
#include <onex-kernel/serial.h>
#endif
#include <onex-kernel/log.h>

void log_init()
{
  APP_ERROR_CHECK(NRF_LOG_INIT(NULL));
}

//#define LOG_BUF_SIZE 1024
//static const char log_buf[LOG_BUF_SIZE];

int log_write(const char* fmt, ...)
{
#ifdef HAS_SERIAL
//size_t n=snprintf((char*)log_buf, LOG_BUF_SIZE, "LOG: %s", fmt);
//if(n>=LOG_BUF_SIZE) n=LOG_BUF_SIZE-1;
  va_list args;
  va_start(args, fmt);
  int r=serial_vprintf(fmt /*log_buf*/, args);
  va_end(args);
  return r;
#else
  return 0;
#endif
}


