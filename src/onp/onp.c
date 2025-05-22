
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <onex-kernel/mem.h>
#include <onex-kernel/time.h>
#include <onex-kernel/log.h>
#include <onex-kernel/serial.h>
#include <onex-kernel/radio.h>
#include <onex-kernel/ipv6.h>

#include "onn.h"
#include "onp.h"

#define VERBOSE_ONP_LOGGING_REMOVE_ME_LATER true

static void on_connect(char* channel);
static void connect_time_cb(void* connected_channel);
static bool handle_recv(uint16_t size, char* channel);
static void send(char* channel);
static void log_sent(char* prefix, uint16_t size, char* channel);
static void log_recv(char* prefix, uint16_t size, char* channel, object* o, char* uid);

extern void onn_recv_observe(char* uid, char* dev);
extern bool onn_recv_object(object* n);

static list* channels=0;
static list* ipv6_groups=0;
static list* serial_ttys=0;
static char* test_uid_prefix=0;

static properties*    device_to_channel = 0;
static volatile list* connected_channels = 0;
static volatile int   num_waiting_on_connect=0;


static void set_channel_of_device(char* device, char* channel){
  properties_ins_setwise(device_to_channel, device, channel);
}

// REVISIT device<s>?? channel<s>? do each channel not #1!
static char* channel_of_device(char* devices){
  list* channels = (list*)properties_get(device_to_channel, devices);
  char* channel = value_string(list_get_n(channels, 1));
  return channel? channel: "all";
}

static bool onp_channel_serial  = false;
static bool onp_channel_radio   = false;
static bool onp_channel_ipv6    = false;
static bool onp_channel_forward = false;

#define MAX_PEERS 32

void channel_on_recv(bool connect, char* channel) {
  if(connect) on_connect(channel);
}

void onp_init(properties* config) {

  channels    = properties_get(config, "channels");
  ipv6_groups = properties_get(config, "ipv6_groups");
  serial_ttys = properties_get(config, "serial_ttys");

  test_uid_prefix=value_string(properties_get(config, "test-uid-prefix"));

  onp_channel_serial = list_has_value(channels,"serial");
  onp_channel_radio  = list_has_value(channels,"radio");
  onp_channel_ipv6   = list_has_value(channels,"ipv6");

  onp_channel_forward = (onp_channel_radio && onp_channel_serial)          ||
                        (onp_channel_ipv6  && onp_channel_serial)          ||
                        (list_size(ipv6_groups)+list_size(serial_ttys) >=2);

  device_to_channel  = properties_new(MAX_PEERS);
  connected_channels = list_new(MAX_PEERS);

  if(onp_channel_serial) serial_init(serial_ttys, 9600, channel_on_recv);
  if(onp_channel_radio)  radio_init(channel_on_recv);
  if(onp_channel_ipv6)   ipv6_init(ipv6_groups, channel_on_recv);

  if(onp_channel_serial)  log_write("ONP channel serial\n");
  if(onp_channel_radio)   log_write("ONP channel radio\n");
  if(onp_channel_ipv6)    log_write("ONP channel IPv6\n");
  if(onp_channel_forward) log_write("ONP forwarding, PCR\n");
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

bool onp_loop() {
  bool ka=true;
  uint16_t size=0;
  uint8_t pkts=0;
  if(onp_channel_serial){
    if(serial_available() >1500) log_write("avail=%d!\n", serial_available());
    while(true){
      size = serial_read(recv_buff, RECV_BUFF_SIZE-1);
    ; if(!size) break;
      pkts++;
      ka = handle_recv(size,"serial") || ka;
    }
  }
  if(onp_channel_radio){
    if(radio_available() >1500) log_write("avail=%d!\n", radio_available());
    while(true){
      size = radio_read(recv_buff, RECV_BUFF_SIZE-1);
    ; if(!size) break;
      pkts++;
      ka = handle_recv(size,"radio") || ka;
    }
  }
  if(onp_channel_ipv6){
    for(int i=1; i<=list_size(ipv6_groups); i++){
      char* group = value_string(list_get_n(ipv6_groups, i));
      size = ipv6_read(group, recv_buff, RECV_BUFF_SIZE-1);
    ; if(!size) continue;
      char channel[256]; snprintf(channel, 256, "ipv6-%s", group);
      ka = handle_recv(size,channel) || ka;
    }
  }
  if(list_size(connected_channels)){

    object_to_text(onex_device_object, send_buff,SEND_BUFF_SIZE, OBJECT_TO_TEXT_NETWORK);

    for(int n=1; n<=list_size(connected_channels); n++){

      char* connected_channel = list_get_n(connected_channels, n);
      log_write("connected: %s %d\n", connected_channel, num_waiting_on_connect);
      send(connected_channel);

      num_waiting_on_connect--;
      mem_freestr(connected_channel);
    }
    list_clear(connected_channels, false);
  }
  if(pkts>1 && log_onp) log_write("pkts=%d\n", pkts);
  return ka || num_waiting_on_connect > 0;
}

#define CONNECT_DELAY_MS 1000

void on_connect(char* channel) {

  // REVISIT: free timer once cb called!
  time_start_timer(time_timeout(connect_time_cb, mem_strdup(channel)), CONNECT_DELAY_MS);
  num_waiting_on_connect++;
  time_delay_ms(10);
  log_write("%s%son_connect(%s) %d\n",
             test_uid_prefix? test_uid_prefix: "",
             test_uid_prefix? " ":             "",
             channel,
             num_waiting_on_connect);
}

void connect_time_cb(void* connected_channel) {
  list_add(connected_channels, connected_channel);
}

static bool recv_observe(uint16_t size, char* channel){

  char* u=recv_buff;

  char* obs=u;
  while(*u > ' ') u++;
  if(!*u) return false;
  *u=0;
  if(strcmp(obs, "OBS:")) return false;
  *u=' ';
  u++;

  char* uid=u;
  while(*u > ' ') u++;
  if(!*u) return false;
  *u=0;
  if(!strlen(uid)) return false;
  uid=mem_strdup(uid);
  *u=' ';
  u++;

  char* dvp=u;
  while(*u > ' ') u++;
  if(!*u){ mem_freestr(uid); return false; }
  *u=0;
  if(strcmp(dvp, "Devices:")){ mem_freestr(uid); return false; }
  *u=' ';
  u++;

  char* dev=u;
  while(*u > ' ') u++;
  *u=0;
  if(!strlen(dev)){ mem_freestr(uid); return false; }
  dev=mem_strdup(dev);

  if(!strcmp(object_property(onex_device_object, "UID"), dev)){
    // log_write("Rejecting own OBS: %s\n", dev);
    mem_freestr(uid); mem_freestr(dev);
    return false;
  }
  log_recv("ONP recv", size, channel, 0, uid);

  set_channel_of_device(dev, channel);

  onn_recv_observe(uid,dev);

  mem_freestr(uid); mem_freestr(dev);

  return true;
}

// REVISIT Device<s> above and below!?

static bool recv_object(uint16_t size, char* channel){

  object* n=object_from_text(recv_buff, MAX_OBJECT_SIZE); if(!n) return false;

  char* uid = object_property(n, "UID");     if(!uid){ object_free(n); return false; }
  char* dev = object_property(n, "Devices"); if(!dev){ object_free(n); return false; }

  log_recv("ONP recv", size, channel, n, 0);

  if(!strcmp(object_property(onex_device_object, "UID"), dev)){
    // log_write("Rejecting own device, UID: %s\n", dev);
    object_free(n);
    return false;
  }
  if(is_local(uid)){
    // log_write("Rejecting own object, UID: %s\n", uid);
    object_free(n);
    return false;
  }

  set_channel_of_device(dev, channel);

  if(!onn_recv_object(n)) object_free(n);

  return true;
}

static bool handle_recv(uint16_t size, char* channel) {

  if(recv_buff[size-1]<=' ') recv_buff[size-1]=0; // REVISIT
  else                       recv_buff[size  ]=0; // REVISIT

  if(size>=5 && !strncmp(recv_buff,"OBS: ",5)) return recv_observe(size, channel);
  if(size>=5 && !strncmp(recv_buff,"UID: ",5)) return recv_object( size, channel);

  if(debug_on_serial && !strncmp(channel, "serial", 6)){
    if(log_debug_read(recv_buff, size)) return true;
  }
  log_recv(">>>>>>>>", size, channel, 0, 0);
  return false;
}

void onp_send_observe(char* uid, char* devices) {
  sprintf(send_buff,"OBS: %s Devices: %s", uid, object_property(onex_device_object, "UID"));
  send(channel_of_device(devices));
}

// REVISIT device<s>?? and send for each channel in above and below
void onp_send_object(object* o, char* devices) {
  if(object_is_remote(o)){
    // log_write("%sforwarding remote: %s\n", onp_channel_forward? "": "not ", object_property(o, "UID"));
    if(!onp_channel_forward) return;
  }
  object_to_text(o,send_buff,SEND_BUFF_SIZE,OBJECT_TO_TEXT_NETWORK);
  send(channel_of_device(devices));
}

void send(char* channel){
  if(onp_channel_serial){
    if(!strncmp(channel, "serial", 6) || !strcmp(channel, "all")){
      char* tty = (!strcmp(channel, "all") || !strcmp(channel, "serial"))? "all": channel + 7;
      uint16_t size = serial_write(tty, send_buff, strlen(send_buff));
      log_sent("ONP sent",size,!strcmp(channel, "all")? "serial-all": channel);
    }
  }
  if(onp_channel_radio){
    if(!strncmp(channel, "radio", 5) || !strcmp(channel, "all")){
      char* band = (!strcmp(channel, "all") || !strcmp(channel, "radio"))? "all": channel + 6;
      uint16_t size = radio_write(band, send_buff, strlen(send_buff));
      log_sent("ONP sent",size,!strcmp(channel, "all")? "radio-all": channel);
    }
  }
  if(onp_channel_ipv6){
    if(!strncmp(channel, "ipv6", 4) || !strcmp(channel, "all")){
      char* group = (!strcmp(channel, "all") || !strcmp(channel, "ipv6"))? "all": channel + 5;
      uint16_t size = ipv6_write(group, send_buff, strlen(send_buff));
      log_sent("ONP sent",size,!strcmp(channel, "all")? "ipv6-all": channel);
    }
  }
}

void log_sent(char* prefix, uint16_t size, char* channel) {
  if(!log_onp) return;
  if(log_to_gfx){
    log_write("> %d\n", size);
    if(size< 48) log_write(send_buff);
  }
  else{
    log_write("%s '%s'", prefix, send_buff);
    if(channel) log_write(" to channel %s ", channel);
    log_write(" (%d bytes)\n", size);
  }
}

void log_recv(char* prefix, uint16_t size, char* channel, object* o, char* uid) {
  if(!log_onp) return;
  if(log_to_gfx){
    if(o)   log_write("U:%s\n", object_property_values(o, "is"));
    if(uid) log_write("O:%s\n", uid);
  }
  else{
    log_write("%s '%s'", prefix, recv_buff);
    if(channel) log_write(" from channel %s ", channel);
    log_write(" (%d bytes)\n", size);
  }
}

// -----------------------------------------------------------------------

