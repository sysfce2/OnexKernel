
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
static void connect_time_cb(void* channel);
static bool handle_recv(uint16_t size, char* chansub);
static void send(char* chansub);
static void log_sent(char* prefix, uint16_t size, char* chansub);
static void log_recv(char* prefix, uint16_t size, char* chansub, object* o, observe obs);

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

static properties*    device_to_chansub = 0;
static volatile list* connected_channels = 0;
static volatile int   num_waiting_on_connect=0;

static void set_chansub_of_device(char* device, char* chansub){
  properties_set_ins(device_to_chansub, device, chansub);
}

// REVISIT device<s>?? chansub<s>? do each chansub not #1!
static value* chansub_of_device(char* device){
  list* chansubs = (list*)properties_get(device_to_chansub, device);
  value* chansub = list_get_n(chansubs, 1);
  return chansub? chansub: value_new("all-all"); // REVISIT all-all not freed
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

  onp_channel_ipv6   = list_vals_has(channels,"ipv6");
  onp_channel_serial = list_vals_has(channels,"serial");
  onp_channel_radio  = list_vals_has(channels,"radio");

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
  device_to_chansub  = properties_new(MAX_PEERS);
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
  uint8_t pkts=0;

  if(onp_channel_serial){
    if(serial_available() >1500) log_write("avail=%d!\n", serial_available());
    while(true){
      int16_t size = serial_read(recv_buff, RECV_BUFF_SIZE);
    ; if(size==0) break;
      pkts++;
      ka = handle_recv(size,"serial") || ka;
    }
  }
  if(onp_channel_radio){
    if(radio_available() >1500) log_write("avail=%d!\n", radio_available());
    while(true){
      int16_t size = radio_read(recv_buff, RECV_BUFF_SIZE);
    ; if(size==0) break;
      if(size<0){ log_write("** radio data corrupt %s\n", recv_buff); continue; }
      pkts++;
      ka = handle_recv(size,"radio") || ka;
    }
  }
  if(onp_channel_ipv6){
    // REVISIT: iterate ip6 groups here, but Unix iterates serial ttys within serial_read
    // and doesn't make ONP aware of the sub-channel/chansub of that tty
    for(int i=1; i<=list_size(ipv6_groups); i++){
      char* group = value_string(list_get_n(ipv6_groups, i));
      int16_t size = ipv6_read(group, recv_buff, RECV_BUFF_SIZE);
    ; if(size==0) continue;
      char chansub[256]; snprintf(chansub, 256, "ipv6-%s", group);
      ka = handle_recv(size,chansub) || ka;
    }
  }
  if(pkts>15) log_write("onp_loop pkts=%d\n", pkts);
  return ka;
}

bool handle_connected(){
  if(!list_size(connected_channels)) return num_waiting_on_connect > 0;
  object_to_text(onex_device_object, send_buff,SEND_BUFF_SIZE, OBJECT_TO_TEXT_NETWORK);
  for(int n=1; n<=list_size(connected_channels); n++){
    char* channel = list_get_n(connected_channels, n);
    log_write("connected: %s %d\n", channel, num_waiting_on_connect);
    send(channel);
    num_waiting_on_connect--;
    mem_freestr(channel);
  }
  list_clear(connected_channels, false);
  return num_waiting_on_connect > 0;
}

void send_pending_obs(properties* pending_obs){
  for(uint16_t i=1; i<=properties_size(pending_obs); i++){
    char*  uid     = properties_key_n(pending_obs,i);
    value* chansub = properties_get_n(pending_obs,i);
    observe_uid_to_text(uid, send_buff, SEND_BUFF_SIZE);
    send(value_string(chansub));
  }
  properties_clear(pending_obs, false);
}

void send_pending_obj(properties* pending_obj){
  for(uint16_t i=1; i<=properties_size(pending_obj); i++){
    char*  uid     = properties_key_n(pending_obj,i);
    value* chansub = properties_get_n(pending_obj,i);
    object_uid_to_text( uid, send_buff, SEND_BUFF_SIZE, OBJECT_TO_TEXT_NETWORK);
    send(value_string(chansub));
  }
  properties_clear(pending_obj, false);
}

static uint32_t serial_lt = 0;
static uint32_t radio_lt = 0;
static uint32_t ipv6_lt = 0;

bool handle_all_send(){
  uint32_t ct = time_ms();
  bool ka=false;
  if(onp_channel_serial && ct > serial_lt + 1){
    serial_lt = ct;
    send_pending_obs(serial_pending_obs);
    send_pending_obj(serial_pending_obj);
  }
  if(onp_channel_radio && ct > radio_lt + 1){
    radio_lt = ct;
    send_pending_obs(radio_pending_obs);
    send_pending_obj(radio_pending_obj);
  }
  if(onp_channel_ipv6 && ct > ipv6_lt + 1){
    ipv6_lt = ct;
    send_pending_obs(ipv6_pending_obs);
    send_pending_obj(ipv6_pending_obj);
  }
  return ka;
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

  // REVISIT: using up a timer for every channel; free timer once cb called!
  uint16_t tid = time_timeout(connect_time_cb, mem_strdup(channel));
  time_start_timer(tid, CONNECT_DELAY_MS);
  num_waiting_on_connect++;
  time_delay_ms(10); // REVISIT
  log_write("%s%son_connect(%s) %d\n",
             test_uid_prefix? test_uid_prefix: "",
             test_uid_prefix? " ":             "",
             channel,
             num_waiting_on_connect);
}

void connect_time_cb(void* channel) {
  list_add(connected_channels, channel);
}

static bool recv_observe(uint16_t size, char* chansub){

  observe obs = observe_from_text(recv_buff);

  if(!obs.uid) return false;

  log_recv("ONP recv", size, chansub, 0, obs);

  set_chansub_of_device(obs.dev, chansub);

  onn_recv_observe(obs);

  return true;
}

// REVISIT Device<s> above and below!?

static bool recv_object(uint16_t size, char* chansub){

  object* n=object_from_text(recv_buff, true, MAX_OBJECT_SIZE); if(!n) return false;

  char* uid = object_property(n, "UID");     if(!uid){ object_free(n); return false; }
  char* dev = object_property(n, "Devices"); if(!dev){ object_free(n); return false; }

  log_recv("ONP recv", size, chansub, n, (observe){0,0});

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

  set_chansub_of_device(dev, chansub);

  onn_recv_object(n);

  return true;
}

static bool handle_recv(uint16_t size, char* chansub) {

  if(recv_buff[size-1]<=' ') recv_buff[size-1]=0; // REVISIT
  else                       recv_buff[size  ]=0; // REVISIT

  if(size>=5 && !strncmp(recv_buff,"OBS: ",5)) return recv_observe(size, chansub);
  if(size>=5 && !strncmp(recv_buff,"UID: ",5)) return recv_object( size, chansub);

  if(debug_on_serial && !strncmp(chansub, "serial", 6)){
    if(log_debug_read(recv_buff, size)) return true;
  }
  log_recv(">>>>>>>>", size, chansub, 0, (observe){0,0});
  return false;
}

void set_pending(char* channel, properties* p, char* uid, value* chansub){

  bool prefix_of_chansub_is_channel = !strncmp(value_string(chansub), channel, strlen(channel));
  bool prefix_of_chansub_is_all     = value_is(chansub, "all-all");

  if(!(prefix_of_chansub_is_channel || prefix_of_chansub_is_all)) return;

  value* channel_all = value_fmt("%s-all", channel); // REVISIT: no value_free/ref count

  if(prefix_of_chansub_is_all) chansub = channel_all; // REVISIT: "all-all" not freed

  value* cs=(value*)properties_get(p, uid);

  if(!cs){
    properties_set(p, uid, chansub);
    return;
  }
  if(value_equal(cs, chansub)) return;
  if(value_equal(cs, channel_all)) return;

  properties_set(p, uid, channel_all);
}

void onp_send_observe(char* uid, char* device) {
  value* chansub = chansub_of_device(device);
  set_pending("serial", serial_pending_obs, uid, chansub);
  set_pending("radio",  radio_pending_obs,  uid, chansub);
  set_pending("ipv6",   ipv6_pending_obs,   uid, chansub);
}

// REVISIT device<s>?? and send for each chansub in above and below
void onp_send_object(char* uid, char* device) {
  if(!onp_channel_forward && !is_local(uid)) return;
  value* chansub = chansub_of_device(device);
  set_pending("serial", serial_pending_obj, uid, chansub);
  set_pending("radio",  radio_pending_obj,  uid, chansub);
  set_pending("ipv6",   ipv6_pending_obj,   uid, chansub);
}

void send(char* chansub){ // return false if *_write() couldn't fit data in, etc
  if(onp_channel_serial){
    if(!strncmp(chansub, "serial", 6)){
      char* tty = strlen(chansub) > 6? chansub + 7: "all";
      uint16_t size = serial_write(tty, send_buff, strlen(send_buff));
      log_sent("ONP sent",size,chansub);
    }
  }
  if(onp_channel_radio){
    if(!strncmp(chansub, "radio", 5)){
      char* band = strlen(chansub) > 5? chansub + 6: "all";
      uint16_t size = radio_write(band, send_buff, strlen(send_buff));
      log_sent("ONP sent",size,chansub);
    }
  }
  if(onp_channel_ipv6){
    if(!strncmp(chansub, "ipv6", 4)){
      char* group = strlen(chansub) > 4? chansub + 5: "all";
      uint16_t size = ipv6_write(group, send_buff, strlen(send_buff));
      log_sent("ONP sent",size,chansub);
    }
  }
}

void log_sent(char* prefix, uint16_t size, char* chansub) {
  if(!log_onp) return;
  if(log_to_gfx){
    if(size>=5 && !strncmp(send_buff,"OBS: ",5)){
      log_write("[%s]%d\n", send_buff, size);
    }
    if(size>=5 && !strncmp(send_buff,"UID: ",5)){
      char* isp=strstr(send_buff, "is:"); if(!isp) return;
      isp[60]=0;
      log_write("[%s]%d\n", isp, size);
    }
  }
  else{
    log_write("%s '%s'", prefix, send_buff);
    if(chansub) log_write(" to chansub %s ", chansub);
    log_write(" (%d bytes)\n", size);
  }
}

void log_recv(char* prefix, uint16_t size, char* chansub, object* o, observe obs) {
  if(!log_onp) return;
  if(log_to_gfx){
    if(o)       log_write("U:%s\n", object_property_values(o, "is"));
    if(obs.uid) log_write("O:%s\n", obs.uid);
  }
  else{
    log_write("%s '%s'", prefix, recv_buff);
    if(chansub) log_write(" from chansub %s ", chansub);
    log_write(" (%d bytes)\n", size);
  }
}

// -----------------------------------------------------------------------

