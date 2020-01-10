
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

#define ONP_DEBUG

static void onp_on_connect();
static void handle_recv(char* buff, int size, char* device, uint16_t* fromip);
static void send(char* buff, char* to);
static void log_sent(char* buff, int size, char* to,   uint16_t* toip);

void onf_recv_observe(char* buff, char* device);
void onf_recv_object(char* text, char* device);

void onp_init()
{
#ifdef ONP_CHANNEL_SERIAL
  channel_serial_init(onp_on_connect);
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

void onp_loop()
{
  char buff[256];
  int  size=0;
#ifdef ONP_CHANNEL_SERIAL
  size = channel_serial_recv(buff, 256);
  if(size!= -1){ handle_recv(buff,size,"Serial",0); return; }
#endif
#ifdef ONP_CHANNEL_IPV6
  size = channel_ipv6_recv(buff, 256, single_peer);
  if(size!= -1){ handle_recv(buff,size,0,single_peer); return; }
#endif
}

extern object* device_object;

void onp_on_connect()
{
  log_write("onp_on_connect\n");
  onp_send_object(device_object,0);
}

static void handle_recv(char* buff, int size, char* device, uint16_t* fromip)
{
  buff[size]=0;

#ifdef ONP_DEBUG
  log_write("ONP recv '%s'", buff);
  if(device)    log_write(" from %s ", device);
#ifdef ONP_CHANNEL_IPV6
  if(fromip){ log_write(" from "); channel_ipv6_show_host_and_port(fromip); }
#endif
  log_write(" (%d bytes)\n", size);
#endif

  if(size>=5 && !strncmp(buff,"OBS: ",5)) onf_recv_observe(buff, device);
  if(size>=5 && !strncmp(buff,"UID: ",5)) onf_recv_object(buff, device);
}

void onp_send_observe(char* uid, char* device)
{
  char buff[128];
  sprintf(buff,"OBS: %s", uid);
  send(buff, device);
}

void onp_send_object(object* o, char* device)
{
  char buff[256];
  object_to_text(o,buff,256,OBJECT_TO_TEXT_NETWORK);
  send(buff, device);
}

void send(char* buff, char* device)
{
  int size=0;
#ifdef ONP_CHANNEL_SERIAL
  size = channel_serial_send(buff, strlen(buff));
  log_sent(buff,size,"Serial",0);
  if(size<0) channel_serial_init(onp_on_connect);
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

// -----------------------------------------------------------------------

