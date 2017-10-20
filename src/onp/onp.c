
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <onex-kernel/time.h>
#include <onex-kernel/serial.h>

#include "onp.h"

#ifdef ONP_CHANNEL_SERIAL
#include <channel-serial.h>
#endif

#ifdef ONP_CHANNEL_IPV6
#include <channel-ipv6.h>
#endif

#if !defined(TARGET_MCU_NRF51822) || !defined(ONP_CHANNEL_SERIAL)
#define ONP_DEBUG 1
#endif

static void handle_recv(char* buff, int size, char* from, uint16_t* fromip);
static void handle_send(char* buff, int size, char* to,   uint16_t* toip);

void recv_observe(char* buff);
void recv_object(char* text);

void onp_init()
{
#ifdef ONP_CHANNEL_SERIAL
  channel_serial_init();
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
  char buff[128];
  int  size=0;
#ifdef ONP_CHANNEL_SERIAL
  size = channel_serial_recv(buff, 128);
  if(size!= -1){ handle_recv(buff,size,"Serial",0); return; }
#endif
#ifdef ONP_CHANNEL_IPV6
  size = channel_ipv6_recv(buff, 128, single_peer);
  if(size!= -1){ handle_recv(buff,size,0,single_peer); return; }
#endif
}

static void handle_recv(char* buff, int size, char* from, uint16_t* fromip)
{
  buff[size]=0;

#ifdef ONP_DEBUG
  serial_printf("ONP recv '%s'", buff);
  if(from)    serial_printf(" from %s ", from);
#ifdef ONP_CHANNEL_IPV6
  if(fromip){ serial_printf(" from "); channel_ipv6_show_host_and_port(fromip); }
#endif
  serial_printf(" (%d bytes)\n", size);
#endif

  if(size>=5 && !strncmp(buff,"OBS: ",5)) recv_observe(buff);
  if(size>=5 && !strncmp(buff,"UID: ",5)) recv_object(strdup(buff));
}

char* object_to_text(object* n, char* b, uint8_t s);
static void send(char* buff);

void onp_send_observe(char* uid)
{
  char buff[128];
  sprintf(buff,"OBS: %s", uid);
  send(buff);
}

void onp_send_object(object* o)
{
  char buff[128];
  object_to_text(o,buff,128);
  send(buff);
}

static void send(char* buff)
{
  int size=0;
#ifdef ONP_CHANNEL_SERIAL
  size = channel_serial_send(buff, strlen(buff));
  handle_send(buff,size,"Serial",0);
#endif
#ifdef ONP_CHANNEL_IPV6
  size = channel_ipv6_send(buff, strlen(buff), single_peer);
  handle_send(buff,size,0,single_peer);
#endif
}

static void handle_send(char* buff, int size, char* to, uint16_t* toip)
{
#ifdef ONP_DEBUG
  serial_printf("ONP send '%s'", buff);
  if(to)    serial_printf(" to %s ", to);
#ifdef ONP_CHANNEL_IPV6
  if(toip){ serial_printf(" to "); channel_ipv6_show_host_and_port(toip); }
#endif
  if(size>=0) serial_printf(" (%d bytes)\n", size);
  else        serial_printf(" (failed to send)\n");
#endif
}

// -----------------------------------------------------------------------

