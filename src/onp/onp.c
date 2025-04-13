
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <onex-kernel/mem.h>
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

extern void onn_recv_observe(char* uid, char* dev);
extern void onn_recv_object(object* n);

static list* groups=0;

static properties* device_to_channel = 0;

static void set_channel_of_device(char* device, char* channel){
  properties_ins_setwise(device_to_channel, device, channel);
}

static char* channel_of_device(char* devices){
  list* channels = (list*)properties_get(device_to_channel, devices);
  char* channel = value_string(list_get_n(channels, 1));
  return channel? channel: "all";
}

#define MAX_PEERS 32

void onp_init(list* g) {
  groups = g;
  device_to_channel = properties_new(MAX_PEERS);
#ifdef ONP_CHANNEL_SERIAL
  channel_serial_init(on_connect);
#endif
#ifdef ONP_CHANNEL_RADIO
  channel_radio_init(on_connect);
#endif
#ifdef ONP_CHANNEL_IPV6
  channel_ipv6_init(groups, on_connect);
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
  for(int i=1; i<=list_size(groups); i++){
    char* group = value_string(list_get_n(groups, i));
    size = channel_ipv6_recv(group, recv_buff, RECV_BUFF_SIZE-1);
    char channel[256]; snprintf(channel, 256, "ipv6-%s", group);
    if(size){ handle_recv(size,channel); return true; }
  }
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
  mem_freestr(connect_channel);
  connect_channel = mem_strdup(channel);
  connect_time = time_ms()+1800;
}

void do_connect() {
  if(!connect_time || time_ms() < connect_time ) return;
  connect_time=0;
  object_to_text(onex_device_object,send_buff,SEND_BUFF_SIZE,OBJECT_TO_TEXT_NETWORK);
  send(send_buff, connect_channel);
}

void recv_observe(uint16_t size, char* channel){

  char* u=recv_buff;

  char* obs=u;
  while(*u > ' ') u++;
  if(!*u) return;
  *u=0;
  if(strcmp(obs, "OBS:")) return;
  *u=' ';
  u++;

  char* uid=u;
  while(*u > ' ') u++;
  if(!*u) return;
  *u=0;
  if(!strlen(uid)) return;
  uid=mem_strdup(uid);
  *u=' ';
  u++;

  char* dvp=u;
  while(*u > ' ') u++;
  if(!*u){ mem_freestr(uid); return; }
  *u=0;
  if(strcmp(dvp, "Devices:")){ mem_freestr(uid); return; }
  *u=' ';
  u++;

  char* dev=u;
  while(*u > ' ') u++;
  *u=0;
  if(!strlen(dev)){ mem_freestr(uid); return; }
  dev=mem_strdup(dev);

  if(!strcmp(object_property(onex_device_object, "UID"), dev)){
    // log_write("reject own OBS: %s\n", dev);
    mem_freestr(uid); mem_freestr(dev);
    return;
  }
  log_recv(recv_buff, size, channel);

  set_channel_of_device(dev, channel);

  onn_recv_observe(uid,dev);

  mem_freestr(uid); mem_freestr(dev);
}

void recv_object(uint16_t size, char* channel){

  object* n=object_from_text(recv_buff, MAX_OBJECT_SIZE);
  if(!n) return;
  char* dev = object_property(n, "Devices");
  if(!dev) return;
  if(!strcmp(object_property(onex_device_object, "UID"), dev)){
    // log_write("reject own UID: %s\n", dev);
    return;
  }
  log_recv(recv_buff, size, channel);

  set_channel_of_device(dev, channel);

  onn_recv_object(n);
}

static void handle_recv(uint16_t size, char* channel) {
  if(recv_buff[size-1]<=' ') recv_buff[size-1]=0;
  else                       recv_buff[size  ]=0;
  if(size>=5 && !strncmp(recv_buff,"OBS: ",5)) recv_observe(size, channel);
  if(size>=5 && !strncmp(recv_buff,"UID: ",5)) recv_object( size, channel);
}
#endif

void onp_send_observe(char* uid, char* devices) {
#if defined(ONP_CHANNEL_SERIAL) || defined(ONP_CHANNEL_RADIO) || defined(ONP_CHANNEL_IPV6)
  sprintf(send_buff,"OBS: %s Devices: %s", uid, object_property(onex_device_object, "UID"));
  send(send_buff, channel_of_device(devices));
#endif
}

void onp_send_object(object* o, char* devices) {
#if defined(ONP_CHANNEL_SERIAL) || defined(ONP_CHANNEL_RADIO) || defined(ONP_CHANNEL_IPV6)
  if(object_is_remote(o)) return; // log_write("======>forwarding remote object %s\n", object_property(o, "UID"));
  object_to_text(o,send_buff,SEND_BUFF_SIZE,OBJECT_TO_TEXT_NETWORK);
  send(send_buff, channel_of_device(devices));
#endif
}

#if defined(ONP_CHANNEL_SERIAL) || defined(ONP_CHANNEL_RADIO) || defined(ONP_CHANNEL_IPV6)
void send(char* buff, char* channel){
#ifdef ONP_CHANNEL_SERIAL
  if(!strcmp(channel, "serial") || !strcmp(channel, "all")){
    uint16_t size = channel_serial_send(buff, strlen(buff));
    log_sent(buff,size,"serial");
  }
#endif
#ifdef ONP_CHANNEL_RADIO
  if(!strcmp(channel, "radio") || !strcmp(channel, "all")){
    uint16_t size = channel_radio_send(buff, strlen(buff));
    log_sent(buff,size,"radio");
  }
#endif
#ifdef ONP_CHANNEL_IPV6
  if(!strncmp(channel, "ipv6-", 5) || !strcmp(channel, "all")){
    char* group = strcmp(channel, "all")? channel + 5: "all";
    uint16_t size = channel_ipv6_send(group, buff, strlen(buff));
    log_sent(buff,size,strcmp(channel, "all")? channel: "ipv6-all");
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

