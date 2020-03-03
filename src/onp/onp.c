
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <onex-kernel/time.h>
#include <onex-kernel/log.h>

#include "onf.h"
#include "onp.h"

#ifdef ONP_CHANNEL_SERIAL
#include <channel-serial.h>
#endif

#ifdef ONP_CHANNEL_IPV6
#include <channel-ipv6.h>
#endif

#define xONP_DEBUG

#if defined(ONP_CHANNEL_SERIAL) || defined(ONP_CHANNEL_IPV6)
static void on_connect(char* channel);
static void do_connect(char* channel);
static void handle_recv(int size, char* channel, uint16_t* fromip);
static void send(char* buff, char* to);
static void log_sent(char* buff, int size, char* to,   uint16_t* toip);
#endif

void onf_recv_observe(char* text, char* channel);
void onf_recv_object(char* text, char* channel);

void onp_init()
{
#ifdef ONP_CHANNEL_SERIAL
  channel_serial_init(on_connect);
#endif
#ifdef ONP_CHANNEL_IPV6
  channel_ipv6_init();
#endif
}

#ifdef ONP_CHANNEL_IPV6
#ifdef TARGET_LINUX
uint16_t single_peer[] = { 0x2002, 0xd417, 0x1f9e, 0x1234, 0x5e51, 0x4fff, 0xfe7c, 0x1111, 9418 };
#else
uint16_t single_peer[] = { 0x2002, 0xd417, 0x1f9e, 0x1234, 0x5e51, 0x4fff, 0xfe7c, 0x3535, 9418 };
#endif
#endif

#define RECV_BUFF_SIZE 1024
#define SEND_BUFF_SIZE 1024

char recv_buff[RECV_BUFF_SIZE];
char send_buff[SEND_BUFF_SIZE];

static char*    connect_channel=0;
static uint32_t connect_count=0;

void onp_loop()
{
#if defined(ONP_CHANNEL_SERIAL) || defined(ONP_CHANNEL_IPV6)
  int  size=0;
#ifdef ONP_CHANNEL_SERIAL
  size = channel_serial_recv(recv_buff, RECV_BUFF_SIZE-1); // spare for term 0
  if(size){ handle_recv(size,"serial",0); return; }
  if(connect_count){
    connect_count--;
    if(!connect_count) do_connect(connect_channel);
  }
#endif
#ifdef ONP_CHANNEL_IPV6
  size = channel_ipv6_recv(recv_buff, RECV_BUFF_SIZE-1, single_peer);
  if(size){ handle_recv(size,0,single_peer); return; }
#endif
#endif
}

static void onp_send_object_p(object* o, char* channel, bool preamble);

#if defined(ONP_CHANNEL_SERIAL) || defined(ONP_CHANNEL_IPV6)

// TODO: use proper time-delay
void on_connect(char* channel)
{
  connect_channel = channel;
  connect_count = 500000;
}

void do_connect(char* channel)
{
  log_write("do_connect %s\n", channel);
  onp_send_object_p(onex_device_object, channel, true);
}

static void handle_recv(int size, char* channel, uint16_t* fromip)
{
  if(size==255) log_write("ONP receive buffer probably over-filled!");
  recv_buff[size]=0;

#ifdef ONP_DEBUG
  log_write("ONP recv '%s'", recv_buff);
  if(channel)    log_write(" from channel %s ", channel);
#ifdef ONP_CHANNEL_IPV6
  if(fromip){ log_write(" from "); channel_ipv6_show_host_and_port(fromip); }
#endif
  log_write(" (%d bytes)\n", size);
#endif

  if(size>=5 && !strncmp(recv_buff,"OBS: ",5)) onf_recv_observe(recv_buff, channel);
  if(size>=5 && !strncmp(recv_buff,"UID: ",5)) onf_recv_object(recv_buff, channel);
}
#endif

void onp_send_observe(char* uid, char* channel)
{
#if defined(ONP_CHANNEL_SERIAL) || defined(ONP_CHANNEL_IPV6)
  sprintf(send_buff,"OBS: %s Devices: %s", uid, object_property(onex_device_object, "UID"));
  send(send_buff, channel);
#endif
}

void onp_send_object(object* o, char* channel)
{
  onp_send_object_p(o, channel, false);
}

void onp_send_object_p(object* o, char* channel, bool preamble)
{
#if defined(ONP_CHANNEL_SERIAL) || defined(ONP_CHANNEL_IPV6)
  size_t p=preamble? snprintf(send_buff, SEND_BUFF_SIZE, "\n\n"): 0;
  object_to_text(o,send_buff+p,SEND_BUFF_SIZE-p,OBJECT_TO_TEXT_NETWORK);
  send(send_buff, channel);
#endif
}

#if defined(ONP_CHANNEL_SERIAL) || defined(ONP_CHANNEL_IPV6)
void send(char* buff, char* channel)
{
  int size=0;
#ifdef ONP_CHANNEL_SERIAL
  if(!strcmp(channel, "serial")){
    size = channel_serial_send(buff, strlen(buff));
    log_sent(buff,size,"Serial",0);
    if(size<0) channel_serial_init(on_connect);
  }
#endif
#ifdef ONP_CHANNEL_IPV6
  size = channel_ipv6_send(buff, strlen(buff), single_peer);
  log_sent(buff,size,0,single_peer);
#endif
}

void log_sent(char* buff, int size, char* to, uint16_t* toip)
{
#ifdef ONP_DEBUG
  log_write("ONP sent '%s'", buff);
  if(to)    log_write(" to %s ", to);
#ifdef ONP_CHANNEL_IPV6
  if(toip){ log_write(" to "); channel_ipv6_show_host_and_port(toip); }
#endif
  if(size>=0) log_write(" (%d bytes)\n", size);
  else        log_write(" (failed to send)\n");
#endif
}
#endif

// -----------------------------------------------------------------------

