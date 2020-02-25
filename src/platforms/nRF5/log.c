
#include <stdio.h>
#include <stdarg.h>

#include "app_error.h"
#include "nrf_log_ctrl.h"

#include <onex-kernel/serial.h>
#include <onex-kernel/log.h>

void log_init()
{
  ret_code_t ret = NRF_LOG_INIT(NULL); APP_ERROR_CHECK(ret);
}

int log_write(const char* fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  int r=serial_vprintf(fmt, args);
  va_end(args);
  return r;
}


