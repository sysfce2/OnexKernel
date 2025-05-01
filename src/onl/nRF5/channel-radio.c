
#include <onex-kernel/radio.h>
#include <onex-kernel/log.h>
#include <channel-radio.h>

static bool initialised=false;

static connect_cb radio_connect_cb;

void channel_radio_on_recv(bool connect, int8_t rssi){
  if(connect){
    if(radio_connect_cb) radio_connect_cb("radio");
    return;
  }
}

void channel_radio_init(connect_cb radio_connect_cb_) {
  radio_connect_cb = radio_connect_cb_;
  initialised = radio_init(channel_radio_on_recv);
}

uint16_t channel_radio_recv(char* buf, uint16_t size) {
  if(!initialised) return 0;
  return radio_read(buf, size);
}

uint16_t channel_radio_send(char* buf, uint16_t size) {
  if(!initialised) return 0;
  return radio_write(buf, size);
}

