
#include <onex-kernel/radio.h>
#include <onex-kernel/log.h>
#include <onex-kernel/chunkbuf.h>

#include <channel-radio.h>

static volatile bool initialised=false;

static connect_cb radio_connect_cb;

#define RADIO_READ_BUFFER_SIZE 2048

static volatile chunkbuf* radio_read_buf = 0;

void channel_radio_on_recv(char* buf, uint16_t size, int8_t rssi){
  if(!buf){
    if(radio_connect_cb) radio_connect_cb("radio");
    return;
  }
  if(!chunkbuf_write(radio_read_buf, buf, size)){
    log_flash(1,0,0);
    return;
  }
}

void channel_radio_init(connect_cb radio_connect_cb_) {
  radio_read_buf = chunkbuf_new(RADIO_READ_BUFFER_SIZE);
  radio_connect_cb = radio_connect_cb_;
  initialised = radio_init(channel_radio_on_recv);
}

uint16_t channel_radio_recv(char* buf, uint16_t size) {
  if(!initialised || !chunkbuf_current_size(radio_read_buf)) return 0;
  return chunkbuf_read(radio_read_buf, buf, size, '\n');
}

uint16_t channel_radio_send(char* b, uint16_t n) { // REVISIT: n not used!! see other channels
  if(!initialised) return 0;
  return radio_printf("%s\n", b); // REVISIT!!!! this is mental - see other channels
}

