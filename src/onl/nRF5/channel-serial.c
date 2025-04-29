
#include <onex-kernel/serial.h>
#include <onex-kernel/log.h>
#include <onex-kernel/chunkbuf.h>

#include <channel-serial.h>

static volatile bool initialised=false;

static connect_cb serial_connect_cb;

#define SERIAL_READ_BUFFER_SIZE 2048

static volatile chunkbuf* serial_read_buf = 0;

void channel_serial_on_recv(char* buf, uint16_t size) {
  if(!buf){
    if(serial_connect_cb) serial_connect_cb("serial");
    return;
  }
  if(!chunkbuf_write(serial_read_buf, buf, size)){
    log_flash(1,0,0);
    return;
  }
}

void channel_serial_init(list* ttys, connect_cb serial_connect_cb_) {
  serial_read_buf = chunkbuf_new(SERIAL_READ_BUFFER_SIZE);
  serial_connect_cb=serial_connect_cb_;
  initialised = serial_init(ttys, channel_serial_on_recv, 9600);
}

uint16_t channel_serial_recv(char* b, uint16_t l) {
  if(!initialised || !chunkbuf_current_size(serial_read_buf)) return 0;
  return chunkbuf_read(serial_read_buf, b, l, '\n');
}

uint16_t channel_serial_send(char* b, uint16_t n) { // REVISIT: n not used!! see other channels
  if(!initialised) return 0;
  return serial_printf("%s\n", b); // REVISIT!!!! this is mental - see other channels
}

