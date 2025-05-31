
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

static void on_connect(char* channel);
static void connect_time_cb(void* connected_channel);
static bool handle_recv(uint16_t size, char* channel);
static void send(char* channel);
static void log_sent(char* prefix, uint16_t size, char* channel);
static void log_recv(char* prefix, uint16_t size, char* channel, object* o, observe* obs);

static list* channels=0;
static list* ipv6_groups=0;
static list* serial_ttys=0;
static list* radio_bands=0;
static char* test_uid_prefix=0;

static properties*    serial_pending_obs = 0;
static properties*    serial_pending_obj = 0;
static properties*    radio_pending_obs = 0;
static properties*    radio_pending_obj = 0;
static properties*    ipv6_pending_obs = 0;
static properties*    ipv6_pending_obj = 0;

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

void channel_on_recv(bool connect, char* channel) {
  if(connect) on_connect(channel);
}

#define MAX_OBS_PENDING 32
#define MAX_OBJ_PENDING 32
#define MAX_PEERS 32

void onp_init(properties* config) {

  channels    = properties_get(config, "channels");
  ipv6_groups = properties_get(config, "ipv6_groups");
  serial_ttys = properties_get(config, "serial_ttys");
  radio_bands = properties_get(config, "radio_bands");

  test_uid_prefix=value_string(properties_get(config, "test-uid-prefix"));

  onp_channel_ipv6   = list_has_value(channels,"ipv6");
  onp_channel_serial = list_has_value(channels,"serial");
  onp_channel_radio  = list_has_value(channels,"radio");

  onp_channel_forward = (onp_channel_radio && onp_channel_serial)          ||
                        (onp_channel_ipv6  && onp_channel_serial)          ||
                        (list_size(ipv6_groups)+list_size(serial_ttys) >=2);

  if(onp_channel_serial){
    serial_pending_obs = properties_new(MAX_OBS_PENDING);
    serial_pending_obj = properties_new(MAX_OBJ_PENDING);
  }
  if(onp_channel_radio){
    radio_pending_obs = properties_new(MAX_OBS_PENDING);
    radio_pending_obj = properties_new(MAX_OBJ_PENDING);
  }
  if(onp_channel_ipv6){
    ipv6_pending_obs = properties_new(MAX_OBS_PENDING);
    ipv6_pending_obj = properties_new(MAX_OBJ_PENDING);
  }
  device_to_channel  = properties_new(MAX_PEERS);
  connected_channels = list_new(MAX_PEERS);

  if(onp_channel_serial) serial_init(serial_ttys, 9600, channel_on_recv);
  if(onp_channel_radio)  radio_init(radio_bands, channel_on_recv);
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

bool handle_all_recv(){

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
  if(pkts>15) log_write("onp_loop pkts=%d\n", pkts);
  return ka;
}

bool handle_connected(){
  if(!list_size(connected_channels)) return num_waiting_on_connect > 0;
  object_to_text(onex_device_object, send_buff,SEND_BUFF_SIZE, OBJECT_TO_TEXT_NETWORK);
  for(int n=1; n<=list_size(connected_channels); n++){
    char* connected_channel = list_get_n(connected_channels, n);
    log_write("connected: %s %d\n", connected_channel, num_waiting_on_connect);
    send(connected_channel);
    num_waiting_on_connect--;
    mem_freestr(connected_channel);
  }
  list_clear(connected_channels, false);
  return num_waiting_on_connect > 0;
}

void send_all_entries(properties* p, bool obs){
  char* deviceuid = object_property(onex_device_object, "UID");
  for(uint16_t i=1; i<=properties_size(p); i++){
    char* uid     = properties_key_n(p,i);
    char* channel = properties_get_n(p,i);
    if(obs) snprintf(send_buff,SEND_BUFF_SIZE, "OBS: %s Devices: %s", uid, deviceuid);
    else object_to_text(onex_get_from_cache(uid),send_buff,SEND_BUFF_SIZE,OBJECT_TO_TEXT_NETWORK);
    send(channel);             // sets Devices - OBJ vs UID? also parsing OBS!
  }
  properties_clear(p, false);
}

#define SERIAL_SEND_RATE 800
#define RADIO_SEND_RATE  800
#define IPV6_SEND_RATE   50

static uint32_t serial_lt = 0;
static uint32_t radio_lt = 0;
static uint32_t ipv6_lt = 0;

bool handle_all_send(){
  uint32_t ct = time_ms();
  if(onp_channel_serial && ct > serial_lt + SERIAL_SEND_RATE){
    serial_lt = ct;
    send_all_entries(serial_pending_obs,true);
    send_all_entries(serial_pending_obj,false);
  }
  if(onp_channel_radio && ct > radio_lt + RADIO_SEND_RATE){
    radio_lt = ct;
    send_all_entries(radio_pending_obs,true);
    send_all_entries(radio_pending_obj,false);
  }
  if(onp_channel_ipv6 && ct > ipv6_lt + IPV6_SEND_RATE){
    ipv6_lt = ct;
    send_all_entries(ipv6_pending_obs,true);
    send_all_entries(ipv6_pending_obj,false);
  }
  return false;
}

bool onp_loop() {

  bool ka=false;

  ka = handle_all_recv()  || ka;
  ka = handle_connected() || ka;
  ka = handle_all_send()  || ka;

  return ka;
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

  observe* obs = observe_from_text(recv_buff);

  if(!obs) return false;

  log_recv("ONP recv", size, channel, 0, obs);

  set_channel_of_device(obs->dev, channel);

  onn_recv_observe(obs);

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

  onn_recv_object(n);

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

void set_pending(char* propchan, properties* p, char* uid, char* channel){
  if(strncmp(channel, propchan, strlen(propchan)) && strcmp(channel, "all")) return;
  char* ch=(char*)properties_get(p, uid);
  if(!ch) properties_set(p, uid, channel);
  else
  if(strcmp(channel, ch)) log_write("** %s over %s\n", channel, ch);
}

void onp_send_observe(char* uid, char* devices) {
  char* channel = channel_of_device(devices);
  set_pending("serial", serial_pending_obs, uid, channel);
  set_pending("radio",  radio_pending_obs,  uid, channel);
  set_pending("ipv6",   ipv6_pending_obs,   uid, channel);
}

// REVISIT device<s>?? and send for each channel in above and below
void onp_send_object(char* uid, char* devices) {
  if(!onp_channel_forward && !is_local(uid)) return;
  char* channel = channel_of_device(devices);
  set_pending("serial", serial_pending_obj, uid, channel);
  set_pending("radio",  radio_pending_obj,  uid, channel);
  set_pending("ipv6",   ipv6_pending_obj,   uid, channel);
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

void log_recv(char* prefix, uint16_t size, char* channel, object* o, observe* obs) {
  if(!log_onp) return;
  if(log_to_gfx){
    if(o)   log_write("U:%s\n", object_property_values(o, "is"));
    if(obs) log_write("O:%s\n", obs->uid);
  }
  else{
    log_write("%s '%s'", prefix, recv_buff);
    if(channel) log_write(" from channel %s ", channel);
    log_write(" (%d bytes)\n", size);
  }
}

// -----------------------------------------------------------------------

