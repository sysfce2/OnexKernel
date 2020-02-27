
#include <string.h>

#include <onex-kernel/log.h>
#include <onex-kernel/serial.h>

static bool initialised=false;

static serial_recv_cb recv_cb;

void serial_on_recv(unsigned char* b, size_t len)
{
  if(recv_cb) recv_cb(b, len);
}

bool serial_init(serial_recv_cb cb, uint32_t baudrate)
{
  if(!initialised){
    recv_cb = cb;
    initialised=true;
  }
  return true;
}

void serial_cb(serial_recv_cb cb)
{
    recv_cb = cb;
}

extern void serial_send(char* b);

#define PRINT_BUFF_SIZE 1024
char print_buff[PRINT_BUFF_SIZE];

size_t serial_printf(const char* fmt, ...)
{
  if(!initialised) return -1;
  va_list args;
  va_start(args, fmt);
  int n=vsnprintf(print_buff, PRINT_BUFF_SIZE, fmt, args);
  serial_send(print_buff);
  va_end(args);
  return strlen(print_buff);
}



