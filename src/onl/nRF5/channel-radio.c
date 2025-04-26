
#include <onex-kernel/radio.h>
#include <onex-kernel/log.h>
#include <onex-kernel/chunkbuf.h>
#include <channel-radio.h>

static volatile bool initialised=false;

#define RADIO_READ_BUFFER_SIZE 2048

static volatile chunkbuf* radio_read_buf = 0;

void channel_radio_on_recv(int8_t rssi){
  static char buf[256];
  uint8_t size=radio_recv(buf);
  if(!chunkbuf_write(radio_read_buf, buf, size)){
    log_flash(1,0,0);
    return;
  }
}

void channel_radio_init(connect_cb radio_connect_cb) {
  radio_read_buf = chunkbuf_new(RADIO_READ_BUFFER_SIZE);
  initialised=radio_init(channel_radio_on_recv);
  if(radio_connect_cb) radio_connect_cb("radio");
}

uint16_t channel_radio_recv(char* b, uint16_t l) {
  if(!initialised || !chunkbuf_current_size(radio_read_buf)) return 0;
  return chunkbuf_read(radio_read_buf, b, l, '\n');
}

uint16_t channel_radio_send(char* b, uint16_t n) {
  if(!initialised) return 0;
  return radio_printf("%s\n", b);
}

