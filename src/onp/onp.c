
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <onex-kernel/mem.h>
#include <onex-kernel/time.h>
#include <onex-kernel/log.h>

#include "onn.h"
#include "onp.h"

#define VERBOSE_ONP_LOGGING_REMOVE_ME_LATER true

#include <channel-serial.h>
#include <channel-radio.h>
#include <channel-ipv6.h>

static void on_connect(char* channel);
static void do_connect();
static void handle_recv(uint16_t size, char* channel);
static void send(char* buff, char* channel);
static void log_sent(char* buff, uint16_t size, char* channel);
static void log_recv(char* buff, uint16_t size, char* channel);

extern void onn_recv_observe(char* uid, char* dev);
extern void onn_recv_object(object* n);

static list* channels=0;
static list* ipv6_groups=0;

static properties* device_to_channel = 0;

static void set_channel_of_device(char* device, char* channel){
  properties_ins_setwise(device_to_channel, device, channel);
  if(VERBOSE_ONP_LOGGING_REMOVE_ME_LATER) properties_log(device_to_channel);
}

// REVISIT device<s>?? channel<s>? do each channel not #1!
static char* channel_of_device(char* devices){
  list* channels = (list*)properties_get(device_to_channel, devices);
  char* channel = value_string(list_get_n(channels, 1));
  return channel? channel: "all";
}

static bool onp_channel_serial = false;
static bool onp_channel_radio  = false;
static bool onp_channel_ipv6   = false;
static bool forward = false;

#define MAX_PEERS 32

void onp_init(properties* config) {

  channels    = properties_get(config, "channels");
  ipv6_groups = properties_get(config, "ipv6_groups");

  onp_channel_serial = list_has_value(channels,"serial");
  onp_channel_radio  = list_has_value(channels,"radio");
  onp_channel_ipv6   = list_has_value(channels,"ipv6");

  forward = (onp_channel_radio && onp_channel_serial)        ||
            (onp_channel_ipv6  && onp_channel_serial)        ||
            (onp_channel_ipv6  && list_size(ipv6_groups) >= 2);

  if(forward) log_write("!Forwarding, PCR!\n");

  device_to_channel = properties_new(MAX_PEERS);

  if(onp_channel_serial) channel_serial_init(on_connect);
  if(onp_channel_radio)  channel_radio_init(on_connect);
  if(onp_channel_ipv6)   channel_ipv6_init(ipv6_groups, on_connect);
}

#if defined(NRF5)
#define RECV_BUFF_SIZE 1024
#define SEND_BUFF_SIZE 1024
#else
#define RECV_BUFF_SIZE 4096
#define SEND_BUFF_SIZE 4096
#endif

static char recv_buff[RECV_BUFF_SIZE];
static char send_buff[SEND_BUFF_SIZE];

static char*    connect_channel=0;
static uint32_t connect_time=0;

bool onp_loop() {
  bool keep_awake=false;
  uint16_t size=0;
  if(onp_channel_serial){
    size = channel_serial_recv(recv_buff, RECV_BUFF_SIZE-1);
    if(size){ handle_recv(size,"serial"); return true; }
  }
  if(onp_channel_radio){
    size = channel_radio_recv(recv_buff, RECV_BUFF_SIZE-1);
    if(size){ handle_recv(size,"radio"); return true; }
  }
  if(onp_channel_ipv6){
    for(int i=1; i<=list_size(ipv6_groups); i++){
      char* group = value_string(list_get_n(ipv6_groups, i));
      size = channel_ipv6_recv(group, recv_buff, RECV_BUFF_SIZE-1);
      char channel[256]; snprintf(channel, 256, "ipv6-%s", group);
      if(size){ handle_recv(size,channel); return true; }
    }
  }
  do_connect();
  keep_awake=!!connect_time;
  return keep_awake;
}


// REVISIT: use proper time-delay
// REVISIT: one per channel not one for all - second wipes out other
void on_connect(char* channel) {
  // REVISIT: just hacking this so radio gets priority
  if(connect_channel) return;
  mem_freestr(connect_channel);
  connect_channel = mem_strdup(channel);
  connect_time = time_ms()+1800;
  if(VERBOSE_ONP_LOGGING_REMOVE_ME_LATER) log_write("on_connect(%s) @ %ld\n", connect_channel, connect_time);
}

// REVISIT: test is: run button after light on 1234, 4321 only has device
// REVISIT: also, light's observe of button needs to be triggered by that
void do_connect() {
  if(!connect_time || time_ms() < connect_time ) return;
  if(VERBOSE_ONP_LOGGING_REMOVE_ME_LATER) log_write("do_connect(%s) @ %ld\n", connect_channel, connect_time);
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

// REVISIT Device<s> above and below!?

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

void onp_send_observe(char* uid, char* devices) {
  sprintf(send_buff,"OBS: %s Devices: %s", uid, object_property(onex_device_object, "UID"));
  send(send_buff, channel_of_device(devices));
}

// REVISIT device<s>?? and send for each channel in above and below
void onp_send_object(object* o, char* devices) {
  if(object_is_remote(o)){
    if(!forward) return;
    log_write("forwarding remote: %s\n", object_property(o, "UID"));
  }
  object_to_text(o,send_buff,SEND_BUFF_SIZE,OBJECT_TO_TEXT_NETWORK);
  send(send_buff, channel_of_device(devices));
}

void send(char* buff, char* channel){
  // REVISIT: should be OK in serial, radio, ipv6 order, but isn't
  if(onp_channel_serial){
    if(!strcmp(channel, "serial") || !strcmp(channel, "all")){
      uint16_t size = channel_serial_send(buff, strlen(buff));
      log_sent(buff,size,"serial");
    }
  }
  if(onp_channel_radio){
    if(!strcmp(channel, "radio") || !strcmp(channel, "all")){
      uint16_t size = channel_radio_send(buff, strlen(buff));
      log_sent(buff,size,"radio");
    }
  }
  if(onp_channel_ipv6){
    if(!strncmp(channel, "ipv6-", 5) || !strcmp(channel, "all")){
      char* group = strcmp(channel, "all")? channel + 5: "all";
      uint16_t size = channel_ipv6_send(group, buff, strlen(buff));
      log_sent(buff,size,strcmp(channel, "all")? channel: "ipv6-all");
    }
  }
}

void log_sent(char* buff, uint16_t size, char* channel) {
#if defined(LOG_TO_GFX) // || defined(LOG_TO_SERIAL)
  log_write("> %d\n", size);
#else
  log_write("ONP sent '%s'", buff);
  if(channel) log_write(" to channel %s ", channel);
  log_write(" (%d bytes)\n", size);
#endif
}

void log_recv(char* buff, uint16_t size, char* channel) {
#if defined(LOG_TO_GFX) // || defined(LOG_TO_SERIAL)
  log_write("< %d\n", size);
#else
  log_write("ONP recv '%s'", buff);
  if(channel) log_write(" from channel %s ", channel);
  log_write(" (%d bytes)\n", size);
#endif
}

// -----------------------------------------------------------------------

