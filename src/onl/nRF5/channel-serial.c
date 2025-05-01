#include <onex-kernel/serial.h>
#include <onex-kernel/log.h>
#include <channel-serial.h>

static bool initialised=false;

static connect_cb serial_connect_cb;

void channel_serial_on_recv(bool connect) {
  if(connect){
    if(serial_connect_cb) serial_connect_cb("serial");
    return;
  }
}

void channel_serial_init(list* ttys, connect_cb serial_connect_cb_) {
  serial_connect_cb = serial_connect_cb_;
  initialised = serial_init(ttys, 9600, channel_serial_on_recv);
}

uint16_t channel_serial_recv(char* buf, uint16_t size) {
  if(!initialised) return 0;
  return serial_read(buf, size);
}

uint16_t channel_serial_send(char* buf, uint16_t size) {
  if(!initialised) return 0;
  return serial_write(buf, size);
}

