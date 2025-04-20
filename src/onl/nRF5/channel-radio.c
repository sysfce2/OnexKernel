
#include <nRF5/m-class-support.h>

#include <onex-kernel/radio.h>
#include <onex-kernel/log.h>
#include <channel-radio.h>

static volatile bool initialised=false;

static channel_radio_connect_cb connect_cb;

#define RADIO_BUFFER_SIZE 2048

static volatile char     buffer[RADIO_BUFFER_SIZE];
static volatile uint16_t current_write=0;
static volatile uint16_t current_read=0;

static uint16_t data_available() {
  int16_t da=(int16_t)current_write-(int16_t)current_read;
  return da >= 0? da: da+RADIO_BUFFER_SIZE;
}

void channel_radio_on_recv(int8_t rssi){

  static char ch[256];
  uint8_t n=radio_recv(ch);

  log_write("channel_radio_on_recv(rssi=%d): %d/%d octets \n", rssi, n, strlen(ch));

  for(uint16_t i=0; i<n; i++){
    if(data_available()==RADIO_BUFFER_SIZE-1){
      log_write("channel radio recv buffer full!\n");
      return;
    }
    buffer[current_write++]=ch[i];
    if(current_write==RADIO_BUFFER_SIZE) current_write=0;
  }
}

void channel_radio_init(channel_radio_connect_cb cb) {
  connect_cb=cb;
  initialised=radio_init(channel_radio_on_recv);
  if(connect_cb) connect_cb("radio");
}

uint16_t channel_radio_recv(char* b, uint16_t l) {
  if(!initialised || !data_available()) return 0;

  uint16_t cr=current_read;
  uint16_t da=data_available();
  uint16_t s=0;
  while(true){
    char d=buffer[cr];
    if(d=='\r' || d=='\n') break;
    da--; if(!da) return 0;
    cr++; if(cr==RADIO_BUFFER_SIZE) cr=0;
    s++;
    if(s==l){
      log_write("channel_radio_recv can fill all of the available buffer without hitting EOL!\n");
      return 0;
    }
  }

  uint16_t size=0;
  while(data_available() && size<l){
    b[size++]=buffer[current_read++];
    if(current_read==RADIO_BUFFER_SIZE) current_read=0;
    if(b[size-1]=='\r' || b[size-1]=='\n'){
      if(buffer[current_read]=='\n') continue;
      break;
    }
  }
  return size;
}

uint16_t channel_radio_send(char* b, uint16_t n) {
  if(!initialised) return 0;
  if(in_interrupt_context()) log_write("OOPS: channel_radio_send called from interrupt context!!\n");
  return radio_printf("%s\n", b);
}

