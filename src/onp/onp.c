
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
static void do_connect(char* channel);
static void handle_recv(uint16_t size, char* channel, uint16_t* fromip);
static void send(char* buff, char* to);
static void log_sent(char* buff, uint16_t size, char* to, uint16_t* toip);
static void log_recv(char* buff, uint16_t size, char* channel);
#endif

void onn_recv_observe(char* text, char* channel);
void onn_recv_object(char* text, char* channel);

void onp_init()
{
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

#ifdef ONP_CHANNEL_IPV6
#ifdef TARGET_X86
uint16_t single_peer[] = { 0x2002, 0xd417, 0x1f9e, 0x1234, 0x5e51, 0x4fff, 0xfe7c, 0x1111, 9418 };
#else
uint16_t single_peer[] = { 0x2002, 0xd417, 0x1f9e, 0x1234, 0x5e51, 0x4fff, 0xfe7c, 0x3535, 9418 };
#endif
#endif

#if defined(NRF5)
#define RECV_BUFF_SIZE 1024
#define SEND_BUFF_SIZE 1024
#elif defined(TARGET_TEENSY_4)
#define RECV_BUFF_SIZE 1024
#define SEND_BUFF_SIZE 1024
#else
#define RECV_BUFF_SIZE 4096
#define SEND_BUFF_SIZE 4096
#endif

#if defined(ONP_CHANNEL_SERIAL) || defined(ONP_CHANNEL_RADIO) || defined(ONP_CHANNEL_IPV6)
static char recv_buff[RECV_BUFF_SIZE];
static char send_buff[SEND_BUFF_SIZE];
#endif

#if defined(ONP_CHANNEL_SERIAL) || defined(ONP_CHANNEL_RADIO) || defined(ONP_CHANNEL_IPV6)
static char*    connect_channel=0;
static uint32_t connect_time=0;
#endif

bool onp_loop()
{
  bool keep_awake=false;
#if defined(ONP_CHANNEL_SERIAL) || defined(ONP_CHANNEL_RADIO) || defined(ONP_CHANNEL_IPV6)
  uint16_t size=0;
#ifdef ONP_CHANNEL_SERIAL
  keep_awake=!!connect_time;
  size = channel_serial_recv(recv_buff, RECV_BUFF_SIZE-1); // spare for term 0
  if(size){ handle_recv(size,"serial",0); return true; }
  if(connect_time && time_ms() >= connect_time ){
    connect_time=0;
    do_connect(connect_channel);
  }
#endif
#ifdef ONP_CHANNEL_RADIO
  keep_awake=!!connect_time;
  size = channel_radio_recv(recv_buff, RECV_BUFF_SIZE-1); // spare for term 0
  if(size){ handle_recv(size,"radio",0); return true; }
  if(connect_time && time_ms() >= connect_time ){
    connect_time=0;
    do_connect(connect_channel);
  }
#endif
#ifdef ONP_CHANNEL_IPV6
  keep_awake=!!connect_time;
  size = channel_ipv6_recv(recv_buff, RECV_BUFF_SIZE-1, single_peer);
  if(size){ handle_recv(size,0,single_peer); return true; }
  if(connect_time && time_ms() >= connect_time ){
    connect_time=0;
    do_connect(connect_channel);
  }
#endif
#endif
  return keep_awake;
}

#if defined(ONP_CHANNEL_SERIAL) || defined(ONP_CHANNEL_RADIO) || defined(ONP_CHANNEL_IPV6)

// TODO: use proper time-delay
void on_connect(char* channel)
{
  connect_channel = channel;
  connect_time = time_ms()+1800;
}

void do_connect(char* channel)
{
  onp_send_object(onex_device_object, channel);
}

static void handle_recv(uint16_t size, char* channel, uint16_t* fromip)
{
  recv_buff[size]=0;

  log_recv(recv_buff, size, channel);

  if(size>=5 && !strncmp(recv_buff,"OBS: ",5)) onn_recv_observe(recv_buff, channel);
  if(size>=5 && !strncmp(recv_buff,"UID: ",5)) onn_recv_object(recv_buff, channel);
}
#endif

void onp_send_observe(char* uid, char* channel)
{
#if defined(ONP_CHANNEL_SERIAL) || defined(ONP_CHANNEL_RADIO) || defined(ONP_CHANNEL_IPV6)
  sprintf(send_buff,"OBS: %s Devices: %s", uid, object_property(onex_device_object, "UID"));
  send(send_buff, channel);
#endif
}

void onp_send_object(object* o, char* channel)
{
  if(object_is_remote(o)) return;
#if defined(ONP_CHANNEL_SERIAL) || defined(ONP_CHANNEL_RADIO) || defined(ONP_CHANNEL_IPV6)
  object_to_text(o,send_buff,SEND_BUFF_SIZE,OBJECT_TO_TEXT_NETWORK);
  send(send_buff, channel);
#endif
}

#if defined(ONP_CHANNEL_SERIAL) || defined(ONP_CHANNEL_RADIO) || defined(ONP_CHANNEL_IPV6)
void send(char* buff, char* channel){

  uint16_t size=0;
#ifdef ONP_CHANNEL_SERIAL
  if(!strcmp(channel, "serial") || !strcmp(channel, "all-channels")){
    size = channel_serial_send(buff, strlen(buff));
    log_sent(buff,size,channel,0);
  }
#endif
#ifdef ONP_CHANNEL_RADIO
  if(!strcmp(channel, "radio") || !strcmp(channel, "all-channels")){
    size = channel_radio_send(buff, strlen(buff));
    log_sent(buff,size,channel,0);
  }
#endif
#ifdef ONP_CHANNEL_IPV6
  if(!strcmp(channel, "ipv6") || !strcmp(channel, "all-channels")){
    size = channel_ipv6_send(buff, strlen(buff), single_peer);
    log_sent(buff,size,0,single_peer);
  }
#endif
}

void log_sent(char* buff, uint16_t size, char* to, uint16_t* toip)
{
#ifdef ONP_DEBUG
#if defined(LOG_TO_GFX) || (defined(LOG_TO_SERIAL) && defined(ONP_OVER_SERIAL))
  log_write("> %d\n", size);
#else
  log_write("ONP sent '%s'", buff);
  if(to)    log_write(" to %s ", to);
#ifdef ONP_CHANNEL_IPV6
  if(toip){ log_write(" to "); channel_ipv6_show_host_and_port(toip); }
#endif
  log_write(" (%d bytes)\n", size);
#endif
#endif
}

void log_recv(char* buff, uint16_t size, char* channel)
{
#ifdef ONP_DEBUG
#if defined(LOG_TO_GFX) || (defined(LOG_TO_SERIAL) && defined(ONP_OVER_SERIAL))
  log_write("< %d\n", size);
#else
  log_write("ONP recv '%s'", buff);
  if(channel) log_write(" from channel %s ", channel);
#ifdef ONP_CHANNEL_IPV6
  if(fromip){ log_write(" from "); channel_ipv6_show_host_and_port(fromip); }
#endif
  log_write(" (%d bytes)\n", size);
#endif
#endif
}
#endif

// -----------------------------------------------------------------------

