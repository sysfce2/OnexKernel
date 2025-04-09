
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <onex-kernel/time.h>
#include <onex-kernel/log.h>

#include "onn.h"
#include "onp.h"

#ifdef ONP_CHANNEL_SERIAL
#include <channel-serial.h>
#endif

#ifdef ONP_CHANNEL_RADIO
#include <channel-radio.h>
#endif

#ifdef ONP_CHANNEL_IPV6
#include <channel-ipv6.h>
#endif

#if defined(ONP_CHANNEL_SERIAL) || defined(ONP_CHANNEL_RADIO) || defined(ONP_CHANNEL_IPV6)
static void on_connect(char* channel);
static void do_connect();
static void handle_recv(uint16_t size, char* channel);
static void send(char* buff, char* channel);
static void log_sent(char* buff, uint16_t size, char* channel);
static void log_recv(char* buff, uint16_t size, char* channel);
#endif

void onn_recv_observe(char* text, char* channel);
void onn_recv_object(char* text, char* channel);

void onp_init() {
#ifdef ONP_CHANNEL_SERIAL
  channel_serial_init(on_connect);
#endif
#ifdef ONP_CHANNEL_RADIO
  channel_radio_init(on_connect);
#endif
#ifdef ONP_CHANNEL_IPV6
  channel_ipv6_init(on_connect);
#endif
}

#if defined(NRF5)
#define RECV_BUFF_SIZE 1024
#define SEND_BUFF_SIZE 1024
#else
#define RECV_BUFF_SIZE 4096
#define SEND_BUFF_SIZE 4096
#endif

#if defined(ONP_CHANNEL_SERIAL) || defined(ONP_CHANNEL_RADIO) || defined(ONP_CHANNEL_IPV6)
static char recv_buff[RECV_BUFF_SIZE];
static char send_buff[SEND_BUFF_SIZE];

static char*    connect_channel=0;
static uint32_t connect_time=0;
#endif

bool onp_loop() {
  bool keep_awake=false;
#if defined(ONP_CHANNEL_SERIAL) || defined(ONP_CHANNEL_RADIO) || defined(ONP_CHANNEL_IPV6)
  uint16_t size=0;
#ifdef ONP_CHANNEL_SERIAL
  size = channel_serial_recv(recv_buff, RECV_BUFF_SIZE-1);
  if(size){ handle_recv(size,"serial"); return true; }
#endif
#ifdef ONP_CHANNEL_RADIO
  size = channel_radio_recv(recv_buff, RECV_BUFF_SIZE-1);
  if(size){ handle_recv(size,"radio"); return true; }
#endif
#ifdef ONP_CHANNEL_IPV6
  size = channel_ipv6_recv(recv_buff, RECV_BUFF_SIZE-1);
  if(size){ handle_recv(size,"ipv6"); return true; }
#endif
  do_connect();
  keep_awake=!!connect_time;
#endif
  return keep_awake;
}

#if defined(ONP_CHANNEL_SERIAL) || defined(ONP_CHANNEL_RADIO) || defined(ONP_CHANNEL_IPV6)

// TODO: use proper time-delay
// TODO: one per channel not one for all
void on_connect(char* channel) {
  connect_channel = channel;
  connect_time = time_ms()+1800;
}

void do_connect() {
  if(!connect_time || time_ms() < connect_time ) return;
  connect_time=0;
  onp_send_object(onex_device_object, connect_channel);
}

static void handle_recv(uint16_t size, char* channel) {
  recv_buff[size]=0;
  log_recv(recv_buff, size, channel);
  if(size>=5 && !strncmp(recv_buff,"OBS: ",5)) onn_recv_observe(recv_buff, channel);
  if(size>=5 && !strncmp(recv_buff,"UID: ",5)) onn_recv_object(recv_buff, channel);
}
#endif

void onp_send_observe(char* uid, char* devices) {
#if defined(ONP_CHANNEL_SERIAL) || defined(ONP_CHANNEL_RADIO) || defined(ONP_CHANNEL_IPV6)
  sprintf(send_buff,"OBS: %s Devices: %s", uid, object_property(onex_device_object, "UID"));
  send(send_buff, channel);
#endif
}

void onp_send_object(object* o, char* devices) {
  if(object_is_remote(o)) return;
#if defined(ONP_CHANNEL_SERIAL) || defined(ONP_CHANNEL_RADIO) || defined(ONP_CHANNEL_IPV6)
  object_to_text(o,send_buff,SEND_BUFF_SIZE,OBJECT_TO_TEXT_NETWORK);
  send(send_buff, channel);
#endif
}

#if defined(ONP_CHANNEL_SERIAL) || defined(ONP_CHANNEL_RADIO) || defined(ONP_CHANNEL_IPV6)
void send(char* buff, char* channel){
#ifdef ONP_CHANNEL_SERIAL
  if(!strcmp(channel, "serial") || !strcmp(channel, "all-channels")){
    uint16_t size = channel_serial_send(buff, strlen(buff));
    log_sent(buff,size,channel);
  }
#endif
#ifdef ONP_CHANNEL_RADIO
  if(!strcmp(channel, "radio") || !strcmp(channel, "all-channels")){
    uint16_t size = channel_radio_send(buff, strlen(buff));
    log_sent(buff,size,channel);
  }
#endif
#ifdef ONP_CHANNEL_IPV6
  if(!strcmp(channel, "ipv6") || !strcmp(channel, "all-channels")){
    uint16_t size = channel_ipv6_send(buff, strlen(buff));
    log_sent(buff,size,channel);
  }
#endif
}

void log_sent(char* buff, uint16_t size, char* channel) {
#ifdef ONP_DEBUG
#if defined(LOG_TO_GFX) // || defined(LOG_TO_SERIAL)
  log_write("> %d\n", size);
#else
  log_write("ONP sent '%s'", buff);
  if(channel) log_write(" to channel %s ", channel);
  log_write(" (%d bytes)\n", size);
#endif
#endif
}

void log_recv(char* buff, uint16_t size, char* channel) {
#ifdef ONP_DEBUG
#if defined(LOG_TO_GFX) // || defined(LOG_TO_SERIAL)
  log_write("< %d\n", size);
#else
  log_write("ONP recv '%s'", buff);
  if(channel) log_write(" from channel %s ", channel);
  log_write(" (%d bytes)\n", size);
#endif
#endif
}
#endif

// -----------------------------------------------------------------------

