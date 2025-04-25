#include <onex-kernel/serial.h>
#include <onex-kernel/log.h>
#include <channel-serial.h>

static bool initialised=false;

static connect_cb serial_connect_cb;

void channel_serial_on_recv(unsigned char* buf, size_t size) {
  if(!buf){
    if(serial_connect_cb) serial_connect_cb("serial");
    return;
  }
}

void channel_serial_init(list* ttys, connect_cb serial_connect_cb_) {
  serial_connect_cb=serial_connect_cb_;
  initialised=serial_init(ttys, channel_serial_on_recv, 9600);
}

uint16_t channel_serial_recv(char* b, uint16_t l) {
  if(!initialised) return 0;
  return serial_recv(b,l);
}

uint16_t channel_serial_send(char* b, uint16_t n) {
  if(!initialised) return 0;
  return serial_printf("%s\n", b);
}

